#include "world.hh"
#include "stopwatch.hh"

#include "graphics.hh"

#include "tuni.hh"
TUNI_WARN_OFF()
#include <QtGlobal>
#include <QDebug>
TUNI_WARN_ON()

#include <random>
#include <iostream>
#include <string>

namespace world {

// actual variables (extern in header file):
std::unique_ptr< world_t > current;
std::unique_ptr< world_t > next;

bool running = true;

// säikeiden yhteinen globaali kello
stopwatch world_clock;

// totuusarvo sille, onko uusi kello jo aloitettu
bool is_stopwatch_executed = false;

// muiden säikeiden kellon päivittämisestä poissulkemiseen käytettävä mutex
std::mutex world_clock_mutex;

// semafori synkronointiin uuden kierroksen aloittamiseksi
std::counting_semaphore waiting_for_tick(0);

namespace {
    // QtCreator might give non-pod warning here, explanation:
    // https://github.com/KDE/clazy/blob/master/docs/checks/README-non-pod-global-static.md
    auto rand_generator = std::bind(std::uniform_int_distribution<>(0,1),std::default_random_engine());

    // glider data is from: https://raw.githubusercontent.com/EsriCanada/gosper_glider_gun/master/gosper.txt
    std::string glider_gun[] = {
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 1 0 1 0 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 1 0 0 0 0 0 0 0 1 0 0 1 0 0 0 0 0 0 0 0 0 0 0 1 1 1 0 1 0 0 1 0 0 1 1 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 1 0 0 0 1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 1 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 1 0 0 0 0 0 1 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 1 1 1 1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 1 0 0 1 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0 0 0 0 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "0000 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
    };
}

void init(void)
{
    // creaate and fill empty world data
    current = std::make_unique<world_t>();
    current->fill(Block::empty);
    next = std::make_unique<world_t>();
    next->fill(Block::empty);

    // create a random starting world
    config::coord_t x = 0;
    config::coord_t y = 0;
    for( x = 0; x < config::width; ++x )
    {
        for( y = 0; y < config::height; ++y )
        {
            if( x < (config::width/2) and y < (config::height/2) )
                continue; // leave space for the glider gun

            bool b = rand_generator();
            if(b) {
                (*current)[xy2array(x,y)] = Block::empty;
            } else {
                (*current)[xy2array(x,y)] = Block::occupied;
            }

        }
    }

    // glider gun data to world data
    const auto glider_pos = 20;
    x = glider_pos;
    y = glider_pos;
    for(auto& line : glider_gun)
    {
        for(char c : line)
        {
            if(c == '1') {
                (*current)[xy2array(x,y)] = Block::occupied;
                ++x;
            } else if(c == '0') {
                (*current)[xy2array(x,y)] = Block::empty;
                ++x;
            } else {
                // all other chars ignored
            }
        }
        x = glider_pos;
        ++y;
    }
}

// lisätty alku- ja loppuarvo säikeen työmäärälle
void next_generation(size_t start, size_t end)
{
    // lukko, jonka avulla luodaan yksi uusi kello per kierros
    std::unique_lock<std::mutex> lock(world_clock_mutex);

    // ensimmäinen lukon saanut säie luo uuden kellon world_clock muuttujaan ja vaihtaa totuusarvon
    // muut säikeet vain käyvät ottamassa lukon ja vapauttavat sen tarkastettuaan totuusarvon
    if (!is_stopwatch_executed) {

        is_stopwatch_executed = true;
        world_clock = stopwatch();
    }

    lock.unlock();

    // muutettu säikeiden ajama alue parametrien start ja end mukaan
    for(config::coord_t x = start; x < end; x++ ) {
        for(config::coord_t y = 0; y < config::height; y++) {
            if( world::running == false) return;  // nothing done if simulation is not running

            auto current_pos = xy2array(x,y);
            auto neighbours = num_neighbours(x,y);

            // https://en.wikipedia.org/wiki/Conway's_Game_of_Life#Rules
            if( (*current)[ current_pos ] == Block::occupied && (neighbours==2 || neighbours==3) ) {
                // 1. stay alive if 2 or 3 neighbours
                (*next)[ current_pos ] = Block::occupied;
            } else if( (*current)[ current_pos ] == Block::empty && neighbours == 3 ) {
                // 2. spawn new if exactly three neighbours
                (*next)[ current_pos ] = Block::occupied;
            } else {
                // 3. else cell will be empty
                (*next)[ current_pos ] = Block::empty;
            }
        }
    }
    // siirretty ajanlaskutulostus ja swap-operaatio run_world_swap metodiin
}

// tulostaa uuden maailman luomiseen käytetyn ajan ja vaihtaa maailmat, sekä
// signaloi grafiikkasäikeelle, että uuden maailman voi piirtää ja jää odottamaan uutta kierrosta
void run_world_swap() {

    std::cerr << "next world created in: " << world_clock.elapsed() << std::endl;
    std::swap( current, next );

    is_stopwatch_executed = false;

    // vaihdetaan totuuarvo, että uuden grafiikan voi ajaa, sekä signaloidaan tästä
    if (!graphics::is_graphics_runnable) {

        graphics::is_graphics_runnable = true;
        graphics::graphics_notice.notify_one();
    }

    // jäädään odottamaan uuden kierroksen alkua, kun ajastin vapauttaa semaforin
    waiting_for_tick.acquire();
}

// apufunktio semaforin vapauttamiselle
void release_world_semaphore() {
    waiting_for_tick.release();
}
} // world
