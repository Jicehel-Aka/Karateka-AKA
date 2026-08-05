// pokitto_compat/PokittoDisplay.h — Pokitto::Display sur l'AKA.
//
// V0.2 (upgrade suite au portage Galaxy Fighter, 2e jeu de reference) :
// remplace le "blit immediat par primitive" (V0.1, valide sur Kong-II) par
// un vrai FRAMEBUFFER INDEXE PERSISTANT (220x176, 4 bits/pixel, 2px/octet),
// exactement comme le Pokitto reel. Necessaire car certains jeux (Galaxy
// Fighter) accedent DIRECTEMENT au framebuffer (PD::screenbuffer), lisent
// PD::width/PD::height comme des champs, et assignent PD::invisiblecolor
// directement (pas seulement via setInvisibleColor()). Cette version reste
// 100% compatible avec Kong-II (drawBitmap/fillRect/etc marchent pareil,
// juste en ecrivant dans le framebuffer au lieu de blitter immediatement).
//
// Toutes les primitives ECRIVENT dans le framebuffer ; present() (appele
// une fois par frame depuis Core::update()) decode l'integralite du
// framebuffer via la palette et fait UN SEUL blit vers l'ecran AKA.
#pragma once
#include <cstdint>
#include <cstddef>

// Constantes de rotation/retournement (trouvees par analyse, Karateka) --
// noms/valeurs du vrai SDK Pokitto, absentes des sources du jeu (comme
// palettePico/fontC64). GLOBALES (hors namespace Pokitto) : le jeu les
// utilise nues (NOROT, FLIPH), pas prefixees (meme piege que fontC64).
enum { NOROT = 0, ROT90 = 1, ROT180 = 2, ROT270 = 3 };
enum { NOFLIP = 0, FLIPH = 1, FLIPV = 2, FLIPHV = 3 };

namespace Pokitto {

constexpr int16_t POK_VIEWPORT_X = 50;   // offset fixe sur l'ecran AKA (320x240)
constexpr int16_t POK_VIEWPORT_Y = 32;

class Display {
public:
    // HYPOTHESE CONFIRMEE PAR CAPTURE (Karateka) : le jeu est ecrit pour un
    // canevas LOGIQUE 110x88 (pas 220x176 comme Kong-II/Galaxy Fighter) --
    // les images de fond (110x88) ET les positions/tailles des sprites
    // (ex: icone d'animation du splash screen a x=32,y=38) sont toutes
    // coherentes UNIQUEMENT si l'ensemble de l'affichage est double 2x a la
    // sortie. Une premiere tentative (doubler seulement l'image de fond)
    // corrigeait sa taille mais pas la position des AUTRES sprites (a
    // coordonnees non nulles) -- confirme que c'est TOUT le canevas qui
    // doit etre traite comme 110x88 puis agrandi 2x globalement, pas
    // seulement certains appels individuels.
    // NOTE : ceci est une copie INDEPENDANTE de pokitto_compat, propre a ce
    // portage (Karateka) -- Kong-II/Galaxy Fighter gardent 220x176 natif.
    // Canevas logique 110x88, agrandi 2x a l'affichage (present()) --
    // CONFIRME necessaire pour la quasi-totalite du jeu (sprites, fond,
    // icones du splash screen) : sans lui, tout apparait a moitie taille et
    // mal positionne. SEULE EXCEPTION : le texte defilant intro/extro
    // (directBitmap), qui dessine en coordonnees NATIVES 220x176 -- gere a
    // part via une file d'attente rejouee apres l'agrandissement 2x dans
    // present() (voir plus bas). Ne pas revenir a 220x176 global : ca casse
    // tout le reste du jeu (deja teste, confirme par capture d'ecran).
    static constexpr int16_t width  = 110;
    static constexpr int16_t height = 88;
    static constexpr int16_t PHYSICAL_W = 220;
    static constexpr int16_t PHYSICAL_H = 176;
    static uint8_t  invisiblecolor;
    static uint8_t* screenbuffer;
    static bool     persistence;

    static void clear();
    static void fillScreen(uint16_t color);
    static void drawPixel(int16_t x, int16_t y);
    static void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
    static void drawRect(int16_t x, int16_t y, int16_t w, int16_t h);
    static void fillRect(int16_t x, int16_t y, int16_t w, int16_t h);
    static void drawColumn(int16_t x, int16_t y0, int16_t y1);

    static void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap);
    static void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, bool flipX, bool flipY);
    // Surcharge trouvee par analyse (Karateka) : PD::drawBitmap(x,y,bitmap,NOROT,FLIPH).
    // Rotation non geree (seul NOROT est utilise dans ce corpus) ; le
    // retournement horizontal (FLIPH) est reellement applique.
    static void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int rotation, int flip);
    // HYPOTHESE (Karateka) : les images de fond chargees depuis la SD
    // (readImage/imgBuffer) font 110x88 -- EXACTEMENT la moitie de l'ecran
    // Pokitto (220x176) -- et sont toujours dessinees en (0,0), coherent
    // avec un fond destine a remplir tout l'ecran via doublement 2x. A
    // confirmer visuellement.
    static void drawBitmapScaled2x(int16_t x, int16_t y, const uint8_t* bitmap);

    static void setCursor(int16_t x, int16_t y);
    static void print(const char* s);
    static void print(char c);
    static void println(const char* s);
    static void setFont(const uint8_t* font);

    static void loadRGBPalette(const uint8_t* palette24);
    static void setColor(uint8_t paletteIndex);
    static void setColor(uint8_t paletteIndex, uint8_t bgPaletteIndex);   // HYPOTHESE : trouve par analyse (Karateka), semantique exacte (fond ?) non confirmee -- fg applique, bg mémorisé sans effet pour l'instant
    static void setInvisibleColor(uint8_t paletteIndex);

    static void clearLCD();   // trouve par analyse (Karateka) -- alias de clear() par defaut, semantique exacte non confirmee

    // HYPOTHESE (Karateka) : tous les appels reels utilisent (bitmap,4,1) sans
    // jamais varier -- probablement des parametres explicites/redondants
    // (profondeur 4bpp + transparence activee) plutot qu'une vraie variation
    // (echelle, etc.).
    // IMPORTANT : contrairement a drawBitmap() (qui dessine dans le canevas
    // LOGIQUE 110x88, agrandi 2x a l'affichage), directBitmap() est utilise
    // UNIQUEMENT par le texte defilant intro/extro, qui est ecrit en
    // coordonnees NATIVES 220x176 (confirme par capture d'ecran : le texte
    // etait tronque quand on le faisait passer par le canevas logique).
    // Implementation : met en file d'attente, rejouee par present() APRES
    // l'agrandissement 2x du reste de l'image, directement en resolution
    // physique -- permet aux deux systemes de coexister dans la meme frame.
    static void directBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int bpp, int transparent);

    static void present();

private:
    static void drawBitmapNativeIntoDecodebuf(int16_t x, int16_t y, const uint8_t* bitmap);

    static uint8_t  s_penColor;
    static uint16_t s_palette[16];
    static const uint8_t* s_font;
    static int16_t  s_cursorX, s_cursorY;

    static void ensureBuffers();
    static void setNibble(int16_t x, int16_t y, uint8_t idx);
    static uint8_t getNibble(int16_t x, int16_t y);
};

} // namespace Pokitto
