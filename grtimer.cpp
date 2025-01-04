#include "world.hh"
#include "graphics.hh"
#include "grtimer.hh"

#include "tuni.hh"
TUNI_WARN_OFF()
#include <QObject>
#include <QDebug>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

TUNI_WARN_ON()


GrTimer::GrTimer( QGraphicsPixmapItem* target )
    : QObject(), target_(target)
{
    qDebug() << "GrTimer started";
    timerID_ = startTimer(config::graphics_tick);
}

void GrTimer::closing()
{
    qDebug() << "GrTimer closing";
    world::running = false;
    killTimer( timerID_ );
}

void GrTimer::timerEvent(QTimerEvent*)
{
    qDebug() << "GrTimer tick";

    // poistettu draw_board()-kutsu, nyt sitä kutsuu mainissa luotu oma grafiikkasäie

    target_->setPixmap( *(graphics::current_pixmap) );
    target_->scene()->update();

    // vapautetaan semafori, jonka jälkeen työsäikeet voivat aloittaa uuden maailman luomisen
    world::release_world_semaphore();
}