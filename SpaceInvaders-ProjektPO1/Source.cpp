#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

const int SZEROKOSC_EKRANU = 600;
const int WYSOKOSC_EKRANU = 800;
const int ROZMIAR_GRACZA = 20;
const int ROZMIAR_PRZECIWNIKA = 20;
const int PROMIEN_POCISKU = 3;
const float PREDKOSC_PRZECIWNIKA = 1.0;
const float PREDKOSC_GRACZA = 5.0;
const float PREDKOSC_POCISKU = 7.0;
const int FPS = 60;

struct ObiektGry {
    float x, y;
    float rozmiar;
    ALLEGRO_COLOR kolor;
};

bool sprawdz_kolizje(const ObiektGry& a, const ObiektGry& b) {
    return a.x < b.x + b.rozmiar && a.x + a.rozmiar > b.x && a.y < b.y + b.rozmiar && a.y + a.rozmiar > b.y;
}

void narysuj_samolot(float x, float y, float rozmiar, ALLEGRO_COLOR kolor) {
    float polowa_rozmiaru = rozmiar / 2;
    float cwierc_rozmiaru = rozmiar / 4;
    al_draw_filled_triangle(x, y, x - polowa_rozmiaru, y + rozmiar, x + polowa_rozmiaru, y + rozmiar, kolor); // Rysowanie trójk¹ta jako korpus samolotu
    al_draw_filled_rectangle(x - polowa_rozmiaru, y + rozmiar / 2, x - polowa_rozmiaru + cwierc_rozmiaru, y + rozmiar, kolor); // Rysowanie lewej skrzyd³a
    al_draw_filled_rectangle(x + polowa_rozmiaru - cwierc_rozmiaru, y + rozmiar / 2, x + polowa_rozmiaru, y + rozmiar, kolor); // Rysowanie prawej skrzyd³a
}

void narysuj_przeciwnika(float x, float y, float rozmiar, ALLEGRO_COLOR kolor) {
    float polowa_rozmiaru = rozmiar / 2;
    float cwierc_rozmiaru = rozmiar / 4;
    al_draw_filled_triangle(x, y, x - polowa_rozmiaru, y - rozmiar, x + polowa_rozmiaru, y - rozmiar, kolor); // Rysowanie trójk¹ta jako korpus przeciwnika
    al_draw_filled_rectangle(x - polowa_rozmiaru, y - rozmiar / 2, x - polowa_rozmiaru + cwierc_rozmiaru, y - rozmiar, kolor); // Rysowanie lewego skrzyd³a
    al_draw_filled_rectangle(x + polowa_rozmiaru - cwierc_rozmiaru, y - rozmiar / 2, x + polowa_rozmiaru, y - rozmiar, kolor); // Rysowanie prawego skrzyd³a
}

bool koliduje_z_innymi_przeciwnikami(const ObiektGry& nowy, const std::vector<ObiektGry>& przeciwnicy) {
    for (const auto& przeciwnik : przeciwnicy) {
        if (sprawdz_kolizje(nowy, przeciwnik)) {
            return true;
        }
    }
    return false;
}

ObiektGry wygeneruj_przeciwnika(const std::vector<ObiektGry>& przeciwnicy) {
    ObiektGry nowy;
    do {
        nowy.x = static_cast<float>(std::rand() % (SZEROKOSC_EKRANU - ROZMIAR_PRZECIWNIKA));
        nowy.y = 0;
        nowy.rozmiar = ROZMIAR_PRZECIWNIKA;
        nowy.kolor = al_map_rgb(255, 0, 0);
    } while (koliduje_z_innymi_przeciwnikami(nowy, przeciwnicy));
    return nowy;
}

enum GameState { MENU, PLAYING, GAME_OVER };

int main() {
    al_init();
    al_install_keyboard();
    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon();
    al_install_audio();
    al_init_acodec_addon();

    ALLEGRO_DISPLAY* wyswietlacz = al_create_display(SZEROKOSC_EKRANU, WYSOKOSC_EKRANU);
    ALLEGRO_EVENT_QUEUE* kolejka_zdarzen = al_create_event_queue();
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / FPS);
    ALLEGRO_FONT* font = al_create_builtin_font();
    ALLEGRO_BITMAP* background = al_load_bitmap("background_image.png");
    ALLEGRO_SAMPLE* background_music = al_load_sample("background_music.wav");

    if (!background) {
        fprintf(stderr, "Failed to load background image!\n");
        return -1;
    }
    if (!background_music) {
        fprintf(stderr, "Failed to load background music!\n");
        return -1;
    }

    al_reserve_samples(1); // Rezerwacja jednego kana³u audio dla muzyki t³a

    al_register_event_source(kolejka_zdarzen, al_get_display_event_source(wyswietlacz));
    al_register_event_source(kolejka_zdarzen, al_get_timer_event_source(timer));
    al_register_event_source(kolejka_zdarzen, al_get_keyboard_event_source());

    ObiektGry gracz = { SZEROKOSC_EKRANU / 2 - ROZMIAR_GRACZA / 2, WYSOKOSC_EKRANU - ROZMIAR_GRACZA - 10, ROZMIAR_GRACZA, al_map_rgb(0, 255, 0) };
    std::vector<ObiektGry> przeciwnicy;
    std::vector<ObiektGry> pociski;

    std::srand(std::time(0));
    for (int i = 0; i < 6; ++i) {
        przeciwnicy.push_back(wygeneruj_przeciwnika(przeciwnicy));
    }

    bool przerysuj = true;
    bool wyjdz = false;
    bool klawisz[2] = { false, false };

    GameState stanGry = MENU;

    int highestScore = 0;
    int score = 0;

    al_play_sample(background_music, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_LOOP, NULL); // Rozpoczêcie odtwarzania muzyki w pêtli

    al_start_timer(timer);

    while (!wyjdz) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(kolejka_zdarzen, &ev);

        // Obs³uga zdarzenia timera
        if (ev.type == ALLEGRO_EVENT_TIMER) {
            if (stanGry == PLAYING) {
                // Obs³uga poruszania siê gracza
                if (klawisz[0] && gracz.x > 0) {
                    gracz.x -= PREDKOSC_GRACZA;
                }
                if (klawisz[1] && gracz.x < SZEROKOSC_EKRANU - ROZMIAR_GRACZA) {
                    gracz.x += PREDKOSC_GRACZA;
                }

                // Poruszanie przeciwników i sprawdzenie warunku przegranej
                for (auto& przeciwnik : przeciwnicy) {
                    przeciwnik.y += PREDKOSC_PRZECIWNIKA;
                    if (przeciwnik.y > WYSOKOSC_EKRANU) {
                        stanGry = GAME_OVER;
                        if (score > highestScore) {
                            highestScore = score;
                        }
                    }
                }

                // Poruszanie pocisków
                for (auto& pocisk : pociski) {
                    pocisk.y -= PREDKOSC_POCISKU;
                }

                // Usuwanie pocisków, które wysz³y poza ekran
                pociski.erase(std::remove_if(pociski.begin(), pociski.end(), [](const ObiektGry& p) { return p.y < 0; }), pociski.end());

                // Sprawdzanie kolizji pocisków z przeciwnikami
                std::vector<ObiektGry> nowi_przeciwnicy;
                std::vector<ObiektGry> nowe_pociski;

                for (auto& przeciwnik : przeciwnicy) {
                    bool trafiony = false;
                    for (auto& pocisk : pociski) {
                        if (sprawdz_kolizje(przeciwnik, pocisk)) {
                            trafiony = true;
                            pocisk.y = -10;
                            score += 10;
                            break;
                        }
                    }
                    if (!trafiony) {
                        nowi_przeciwnicy.push_back(przeciwnik);
                    }
                }

                // Usuwanie trafionych przeciwników i zbieranie nowej listy pocisków
                for (auto& pocisk : pociski) {
                    if (pocisk.y >= 0) {
                        nowe_pociski.push_back(pocisk);
                    }
                }

                // Aktualizacja listy przeciwników i generowanie nowych
                przeciwnicy = nowi_przeciwnicy;
                pociski = nowe_pociski;

                while (przeciwnicy.size() < 6) {
                    przeciwnicy.push_back(wygeneruj_przeciwnika(przeciwnicy));
                }

                przerysuj = true;
            }
        }
        // Obs³uga zdarzenia zamkniêcia okna
        else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            wyjdz = true;
        }
        // Obs³uga wciœniêcia klawisza
        else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (stanGry == MENU) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                    // Rozpoczêcie gry po wciœniêciu spacji
                    stanGry = PLAYING;
                    score = 0;
                    przeciwnicy.clear();
                    pociski.clear();
                    for (int i = 0; i < 6; ++i) {
                        przeciwnicy.push_back(wygeneruj_przeciwnika(przeciwnicy));
                    }
                }
                else if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                    // Wyjœcie z gry po wciœniêciu ESC w menu
                    wyjdz = true;
                }
            }
            else if (stanGry == PLAYING) {
                switch (ev.keyboard.keycode) {
                case ALLEGRO_KEY_LEFT:
                    klawisz[0] = true;
                    break;
                case ALLEGRO_KEY_RIGHT:
                    klawisz[1] = true;
                    break;
                case ALLEGRO_KEY_SPACE:
                    // Strzelanie pociskiem po wciœniêciu spacji
                    pociski.push_back({ gracz.x + ROZMIAR_GRACZA / 2 - PROMIEN_POCISKU / 2, gracz.y, PROMIEN_POCISKU, al_map_rgb(255, 255, 0) }); // Zmiana koloru na ¿ó³ty i zmniejszenie promienia
                    break;
                }
            }
            else if (stanGry == GAME_OVER) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                    // Powrót do menu po wciœniêciu spacji po przegranej grze
                    stanGry = MENU;
                }
            }
        }
        // Obs³uga puszczenia klawisza (do obs³ugi poruszania siê gracza)
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            switch (ev.keyboard.keycode) {
            case ALLEGRO_KEY_LEFT:
                klawisz[0] = false;
                break;
            case ALLEGRO_KEY_RIGHT:
                klawisz[1] = false;
                break;
            }
        }

        // Aktualizacja ekranu jeœli jest coœ do narysowania i kolejka zdarzeñ jest pusta
        if (przerysuj && al_is_event_queue_empty(kolejka_zdarzen)) {
            przerysuj = false;

            al_clear_to_color(al_map_rgb(0, 0, 0));

            if (stanGry == MENU) {
                // Wyœwietlanie menu g³ównego
                al_draw_text(font, al_map_rgb(255, 255, 255), SZEROKOSC_EKRANU / 2, WYSOKOSC_EKRANU / 2 - 20, ALLEGRO_ALIGN_CENTER, "Press SPACE to Play");
                al_draw_text(font, al_map_rgb(255, 255, 255), SZEROKOSC_EKRANU / 2, WYSOKOSC_EKRANU / 2 + 20, ALLEGRO_ALIGN_CENTER, "Press ESC to Exit");
                al_draw_text(font, al_map_rgb(255, 255, 255), SZEROKOSC_EKRANU / 2, WYSOKOSC_EKRANU / 2 + 40, ALLEGRO_ALIGN_CENTER, "LEFT and RIGHT to move SPACE to shoot");
            }
            else if (stanGry == PLAYING) {
                // Wyœwietlanie gry
                al_draw_bitmap(background, 0, 0, 0);
                narysuj_samolot(gracz.x, gracz.y, gracz.rozmiar, gracz.kolor);

                for (const auto& przeciwnik : przeciwnicy) {
                    narysuj_przeciwnika(przeciwnik.x, przeciwnik.y, przeciwnik.rozmiar, przeciwnik.kolor);
                }

                for (const auto& pocisk : pociski) {
                    al_draw_filled_circle(pocisk.x + PROMIEN_POCISKU / 2, pocisk.y + PROMIEN_POCISKU / 2, PROMIEN_POCISKU, pocisk.kolor);
                }

                al_draw_textf(font, al_map_rgb(255, 255, 255), 5, 5, 0, "Score: %d", score);
            }
            else if (stanGry == GAME_OVER) {
                // Wyœwietlanie ekranu po przegranej
                al_draw_text(font, al_map_rgb(255, 0, 0), SZEROKOSC_EKRANU / 2, WYSOKOSC_EKRANU / 2 - 20, ALLEGRO_ALIGN_CENTER, "Game Over");
                al_draw_textf(font, al_map_rgb(255, 255, 255), SZEROKOSC_EKRANU / 2, WYSOKOSC_EKRANU / 2 + 20, ALLEGRO_ALIGN_CENTER, "Your Score: %d", score);
                al_draw_textf(font, al_map_rgb(255, 255, 255), SZEROKOSC_EKRANU / 2, WYSOKOSC_EKRANU / 2 + 40, ALLEGRO_ALIGN_CENTER, "Highest Score: %d", highestScore);
                al_draw_text(font, al_map_rgb(255, 255, 255), SZEROKOSC_EKRANU / 2, WYSOKOSC_EKRANU / 2 + 60, ALLEGRO_ALIGN_CENTER, "Press SPACE to Return to Menu");
            }

            al_flip_display();
        }
    }

    // Zwolnienie zasobów Allegro
    al_destroy_display(wyswietlacz);
    al_destroy_event_queue(kolejka_zdarzen);
    al_destroy_timer(timer);
    al_destroy_font(font);
    al_destroy_bitmap(background);
    al_destroy_sample(background_music);

    return 0;
}

