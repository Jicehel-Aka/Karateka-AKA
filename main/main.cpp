// main.cpp — Point d'entree du portage AKA de Karateka (3e jeu de reference
// AKA Port Studio). Pas de systeme de sauvegarde dans ce jeu (contrairement
// a Kong-II/Galaxy Fighter) : pas de GameCookie ici.
#include "gb_core.h"
#include "gb_graphics.h"
#include "core/input.h"
#include "aka_runtime/aka_runtime.h"
#include "pokitto_compat/Pokitto.h"

#include "game/Game.h"

#include <cstdlib>
#include <ctime>

gb_core     g_core;
gb_graphics gfx;

using PC = Pokitto::Core;
using PD = Pokitto::Display;

// Palette PICO-8 (16 x RGB 8-bit) — fournie par le vrai SDK Pokitto ; le
// jeu ne la definit pas lui-meme dans ses sources.
const uint8_t palettePico[16 * 3] = {
      0,  0,  0,   29, 43, 83,  126, 37, 83,   0,135, 81,
    171, 82, 54,   95, 87, 79,  194,195,199,  255,241,232,
    255,  0, 77,  255,163,  0,  255,236, 39,    0,228, 54,
     41,173,255,  131,118,156,  255,119,168,  255,204,170,
};

Game game;

extern "C" void app_main(void) {
    g_core.init();
    gfx.set_backlight_percent(80);
    gfx.set_refresh_rate(60);

    akaRuntime.begin("karateka");
    static const char* const kControls[] = {
        "CTRL_MOVE", "CTRL_KICK", "CTRL_PUNCH", "CTRL_BLOCK",
        "CTRL_STANCE_UP", "CTRL_STANCE_DOWN", nullptr
    };
    akaRuntime.setControlsKeys(kControls);
    akaRuntime.setCredits("Karateka", "Press Play On Tape", "BSD 3-Clause",
                           "github.com/Press-Play-On-Tape/Karateka_Pokitto");

    PC::begin();
    PD::loadRGBPalette(palettePico);
    PD::persistence = true;
    PD::setColor(5);
    PD::setInvisibleColor(14);
    PC::setFrameRate(40);
    PD::setFont(fontTight);

    srand((unsigned)time(0));

    game.setup();

    // BUG TROUVE ET CORRIGE : contrairement a Kong-II/Galaxy Fighter, le
    // Game::loop() de Karateka appelle DEJA PC::update()/PC::update(true)
    // lui-meme en interne (selon l'etat). Appeler PC::update() ICI EN PLUS
    // provoquait un DOUBLE appel par tour : le second (interne a game.loop())
    // echouait systematiquement le test de cadence (execute quelques
    // microsecondes apres le premier), bloquant pour toujours toute logique
    // dependant de PC::update() apres le tout premier passage (ex :
    // introText() ne progressait plus jamais au-dela de son premier appel).
    // BUG TROUVE ET CORRIGE (menu insensible aux touches) : quand le menu
    // systeme est ouvert, akaRuntime.update() renvoie false et on saute
    // game.loop() -- or c'est UNIQUEMENT depuis l'interieur de game.loop()
    // (via son PC::update()/PC::update(true) interne) que g_keys etait
    // rafraichi pour Karateka. Fix : rafraichir g_keys nous-memes, mais
    // SEULEMENT quand le menu est deja ouvert (donc que game.loop() ne
    // tourne pas) -- sonder aussi pendant le jeu normal risquerait de
    // "manger" un front d'appui bref entre notre sondage et celui, interne,
    // du jeu (les deux se recalculent a chaque pool(), pas consommes a la
    // lecture).
    while (PC::isRunning()) {
        if (akaRuntime.isMenuOpen()) input_poll(g_keys);
        if (!akaRuntime.update(g_keys)) continue;
        if (akaRuntime.helpRequested()) game.requestHelp();
        game.loop();
    }
}
