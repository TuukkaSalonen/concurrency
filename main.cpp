//
// Conway's Game of Life
// https://en.wikipedia.org/wiki/Conway's_Game_of_Life
//
// Course template, Concurrency, spring 2023
// license: Free to use for any purpose in tuni.fi teaching,
// other uses: https://creativecommons.org/licenses/by-nc-sa/4.0/
//
#include "config.hh"
#include "world.hh"
#include "graphics.hh"
#include "grtimer.hh"

#include "tuni.hh"
TUNI_WARN_OFF()
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
TUNI_WARN_ON()

#include <thread>
#include <chrono>

// lisätty barrier
#include <barrier>

// vakiot säikeiden määrälle ja kunkin säikeen työmäärälle
size_t NUMBER_OF_THREADS = 10;
size_t THREAD_WORK = 20;

// barrier, joka kutsuu world::run_world_swap, kun kaikki säikeet ovat valmiita
std::barrier WORLD_BARRIER(NUMBER_OF_THREADS, world::run_world_swap);

// asettaa työsäikeiden työalueen ja hoitaa synkronoinnin barrierin kanssa
void run_world_thread(size_t start, size_t end) {

    while(world::running) {

      world::next_generation(start, end);
      WORLD_BARRIER.arrive_and_wait();
    }
}

// grafiikkasäikeen työalue
void run_graphic_thread() {

    while(world::running) {

        graphics::draw_board();
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // data model of the world
    world::init();

    // Qt pixmap of the world
    graphics::init();

    // Qt graphics scene to show the pixmap
    QGraphicsScene scene;
    QGraphicsPixmapItem grarea(*(graphics::current_pixmap));
    scene.addItem( &grarea );

    // Qt graphics view of the pixmap world (scaled)
    QGraphicsView grview(&scene);
    grview.setCacheMode( QGraphicsView::CacheModeFlag::CacheNone );
    grview.setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    grview.scale(config::scale, config::scale);


    // timer to periodically update the graphics
    GrTimer myTimer( &grarea );
    QObject::connect(&app,SIGNAL(aboutToQuit()),&myTimer,SLOT(closing()));

    // vektori työsäikeille, sekä grafiikkasäie
    std::vector<std::thread> world_threads;
    std::thread graphics_thread(run_graphic_thread);

    // säikeen työmäärän alku- ja loppuarvo, jota päivitetään uusien säikeiden käynnistyksessä
    size_t start = 0;
    size_t end = THREAD_WORK + 1;

    // aloitetaan työsäikeet omilla työmäärillä ja laitetaan ne vektoriin
    for (size_t i = 0; i < NUMBER_OF_THREADS; i++) {
        world_threads.emplace_back(run_world_thread, start, end);
        start = end;
        end += THREAD_WORK;
    }

    grview.show();
    app.exec(); // main thread handles the Qt event loop

    // kerätään työsäikeet ja grafiikkasäie ohjelman suorituksen loputtua
    for (std::thread& thread : world_threads) {
        thread.join();
    }
    graphics_thread.join();

    return EXIT_SUCCESS;
}
