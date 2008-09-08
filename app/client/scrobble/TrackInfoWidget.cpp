/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "TrackInfoWidget.h"
#include "ObservedTrack.h"
#include "PlayerEvent.h"
#include "lib/unicorn/widgets/SpinnerLabel.h"
#include <QtGui>


TrackInfoWidget::TrackInfoWidget()
{
    QVBoxLayout* v = new QVBoxLayout( this );
    v->setMargin( 0 );
    v->addStretch();
    v->addWidget( ui.text = new QLabel );
#ifdef Q_WS_MAC
    v->addSpacing( 11 );
    ui.text->setPalette( QPalette( Qt::white, Qt::black ) ); //Qt bug, it should inherit! TODO report bug
    ui.text->setAttribute( Qt::WA_MacSmallSize );
#else
    v->addSpacing( 4 );
#endif
    
    ui.text->setAlignment( Qt::AlignBottom | Qt::AlignHCenter );
    ui.text->setTextFormat( Qt::RichText );
	
	ui.spinner = new SpinnerLabel( this );
	ui.spinner->hide();
}


void
TrackInfoWidget::clear()
{
	m_track = Track();
	m_cover = QImage();
	ui.text->clear();
	ui.spinner->hide();
	qDeleteAll( findChildren<AlbumImageFetcher*>() );
	update();
}


void
TrackInfoWidget::setTrack( const Track& t )
{
	//TODO for scrobbled tracks we should get the artwork out of the track
	if (m_track.album() != t.album())
	{
		m_cover = QImage();
		update();
		
		qDeleteAll( findChildren<AlbumImageFetcher*>() );
		QObject* o = new AlbumImageFetcher( t.album(), Album::Large );
		connect( o, SIGNAL(finished( QByteArray )), SLOT(onAlbumImageDownloaded( QByteArray )) );
		o->setParent( this );
		
		ui.spinner->show();
	}

	m_track = t;

	// TODO handle bad data
	ui.text->setText( "<div style='margin-bottom:3px'>" + t.artist() + "</div><div><b>" + t.title() );
}


void
TrackInfoWidget::onAlbumImageDownloaded( const QByteArray& data )
{
	if (data.size())
	{
		m_cover.loadFromData( data );
	    qDebug() << "Image dimensions:" << m_cover.size();
	}
	else
		m_cover = QImage( ":/blank/cover.png" ); //TODO blank cover in unicorn

	m_cover = addReflection( m_cover );
	update();

	sender()->deleteLater();
	
	ui.spinner->hide();
}


void
TrackInfoWidget::paintEvent( QPaintEvent* e )
{
#if 0
    QPainter p( this );
    p.setClipRect( e->rect() );
    p.setRenderHint( QPainter::Antialiasing );
    p.setRenderHint( QPainter::SmoothPixmapTransform );

    QLinearGradient g( QPoint(), QPoint( width(), height() ) );
    g.setColorAt( 0, Qt::transparent );
    g.setColorAt( 1, QColor( 0x2b, 0x2b, 0x2b ) );
    p.fillRect( rect(), g );

    if (m_cover.isNull()) return;

    // determine rotated height
    QTransform trans;
    trans.rotate( -27, Qt::YAxis );
    QRectF r1 = rect().translated( -width()/2, -height()/2 );
    qreal const h = trans.inverted().map( QLineF( r1.topLeft(), r1.bottomLeft() ) ).dy();

    // calculate scaling factor
    qreal const scale = h / m_cover.height();
    trans.scale( scale, scale );

    // draw
    p.setTransform( trans * QTransform().translate( height()/2, height()/3 + 10 ) );
    p.drawImage( QPoint( -m_cover.height()/2, -m_cover.height()/3 ), m_cover );
#endif

    if (m_cover.isNull()) return;

    QPainter p( this );
    p.setClipRect( e->rect() );
    p.setRenderHint( QPainter::Antialiasing );
    p.setRenderHint( QPainter::SmoothPixmapTransform );
    QTransform trans;
    qreal const scale = qreal(height()) / m_cover.height();
    trans.scale( scale, scale );
    p.setTransform( trans );

    QPointF f = trans.inverted().map( QPointF( width(), 0 ) );

    f.rx() -= m_cover.width();
    f.rx() /= 2;

    p.drawImage( f, m_cover );
}


QImage //static
TrackInfoWidget::addReflection( const QImage &in )
{
    const uint H = qreal(in.height()) / 3;
	
    QImage out( in.width(), in.height() + H, QImage::Format_ARGB32_Premultiplied );
    QPainter p( &out );
	QImage in2 = in;
	in2.convertToFormat( QImage::Format_ARGB32_Premultiplied );
    p.drawImage( 0, 0, in2 );
    
    QImage reflection = in.copy( 0, in.height() - H, in.width(), H );
    reflection = reflection.mirrored( false, true /*vertical only*/ );
	
    QLinearGradient gradient( QPoint( 0, in.height() ), QPoint( 0, out.height() ) );
    gradient.setColorAt( 0, QColor(0, 0, 0, 100) );
    gradient.setColorAt( 1, Qt::transparent );
	
    p.drawImage( 0, in.height(), reflection );
    p.setCompositionMode( QPainter::CompositionMode_DestinationIn );
    p.fillRect( QRect( QPoint( 0, in.height() ), reflection.size() ), gradient );
	
    return out;
}