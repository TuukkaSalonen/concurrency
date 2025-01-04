#include "world.hh"
#include "graphics.hh"
#include "stopwatch.hh"

#include "tuni.hh"
TUNI_WARN_OFF()
#include <QPixmap>
#include <QPainter>
TUNI_WARN_ON()

#include <iostream>

// lisätty mutex ja semafori
#include <mutex>
#include <semaphore>

namespace graphics {

QPixmap* current_pixmap = nullptr;
QPixmap* next_pixmap = nullptr;

// ehtomuuttuja, joka odottaa signaalia maailman luomisesta
std::condition_variable graphics_notice;

// mutex ehtomuuttujaa varten
std::mutex graphics_mutex;

// totuusarvo, voiko uuden grafiikan piirtää
bool is_graphics_runnable = false;

void init(void) {
    current_pixmap = new QPixmap( config::width, config::height );
    current_pixmap->fill( bgColor );
    next_pixmap = new QPixmap( config::width, config::height );
    next_pixmap->fill( bgColor );
}

void draw_board()
{
    // lisätty lukko ja liitetty se ehtomuuttujaan, jossa odotetaan signaalia, voiko uuden maailman piirtää
    std::unique_lock<std::mutex> lock (graphics_mutex);
    graphics_notice.wait(lock, []() {return is_graphics_runnable == true;});

    QPainter painter(next_pixmap);
    stopwatch clock;

    for(config::coord_t x=0; x<config::width; x++)
    {
        for(config::coord_t y=0; y<config::height; y++ ) {
            bool isEmpty = (*world::current)[ world::xy2array(x,y) ] == world::Block::empty;
            if( isEmpty ) {
                painter.setPen(bgColor);
                painter.drawPoint(x,y);
            } else {
                painter.setPen(fgColor);
                painter.drawPoint(x,y);

            }
        }
    }
    std::cerr << "graphics pixmap created in: " << clock.elapsed() << std::endl;

    std::swap( next_pixmap, current_pixmap );

    // asetetaan piirron jälkeen totuusarvo
    is_graphics_runnable = false;
}
} // graphics
