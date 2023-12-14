#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <stack>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include "CycleTimer.h"
#include "pthread.h"

const int rozmiar_sudoku = 9;
int rozmiar_kwadratu = 3;
int glebokosc_rekursji; //Liczba miejsc, które wątek wypełnia przed odłożeniiem na stos
static int ilosc_watkow;

class Plansza {
public:
      int plansza[rozmiar_sudoku][rozmiar_sudoku]; //tablica sudoku
      Plansza() {};

      void wyswietl_tablice()
      {
          for (int i = 0; i < rozmiar_sudoku; i++) {
              for (int j = 0; j < rozmiar_sudoku; j++)
                  std::cout << plansza[i][j] << " ";
              std::cout << std::endl;
          }
          std::cout << std::endl;
      }

      int ile_wszystkich_pustych() {
          int cnt = 0;
          for (int i = 0; i < rozmiar_sudoku; i++)
              for (int j = 0; j < rozmiar_sudoku; j++)
                  cnt += (plansza[i][j] == 0);
          return cnt;
      }
};

bool czy_brak_konfliktow(int matrix[rozmiar_sudoku][rozmiar_sudoku], int wiersz, int kolumna, int numer) {

    for (int i = 0; i < rozmiar_sudoku; i++) {
        if (matrix[i][kolumna] == numer)
            return false;
    }

    for (int j = 0; j < rozmiar_sudoku; j++) {
        if (matrix[wiersz][j] == numer)
            return false;
    }

    for (int i = 0; i < rozmiar_kwadratu; i++) {
        for (int j = 0; j < rozmiar_kwadratu; j++) {
            if (matrix[(wiersz / rozmiar_kwadratu) * rozmiar_kwadratu + i][(kolumna / rozmiar_kwadratu) * rozmiar_kwadratu + j] == numer)
                return false;
        }
    }

    return true;
}

int znajdz_nastepna_pusta(int matrix[rozmiar_sudoku][rozmiar_sudoku], int start) {
    int i;
    for (i = start; i < rozmiar_sudoku * rozmiar_sudoku; i++) {
        if (matrix[i / rozmiar_sudoku][i % rozmiar_sudoku] == 0) {
            return i;
        }
    }
    return i;
}
std::mutex mtx;

bool wyszukiwanie_wsteczne(std::stack < std::pair < int, Plansza >>& stk, Plansza b, int indeks, int glebokosc) {
    int wiersz = indeks / rozmiar_sudoku;
    int kolumna = indeks % rozmiar_sudoku;
    //b.wyswietl_tablice();
    if (indeks >= rozmiar_sudoku * rozmiar_sudoku) { //jesli osiagnieto koniec planszy (rozwiazanie zostalo znalezione)
        mtx.lock();
        stk.push(std::pair < int, Plansza >(indeks, b));
        mtx.unlock();
        return true;
    }

    if (!glebokosc) { //jesli osiagnieto maksymalna glebokosci rekursji
        mtx.lock();
        stk.push(std::pair < int, Plansza >(indeks, b));
        mtx.unlock();
        return false;
    }

    for (int k = 1; k <= rozmiar_sudoku; k++) {
        if (czy_brak_konfliktow(b.plansza, wiersz, kolumna, k)) {
            b.plansza[wiersz][kolumna] = k;
            if (wyszukiwanie_wsteczne(stk, b, znajdz_nastepna_pusta(b.plansza, indeks + 1), glebokosc - 1))
                return true;
        }
    }

    return false;
}

void backtracking_lokalny(Plansza& plansza_sudoku) {
    std::stack < std::pair < int, Plansza >> stk;
    std::deque < std::pair < int, Plansza >> vec;
    std::vector < std::thread > threads;

    bool czy_ukonczono = false;

    Plansza tmp(plansza_sudoku);

    stk.push(std::pair < int, Plansza >(znajdz_nastepna_pusta(tmp.plansza, 0), tmp));
    int indeks = znajdz_nastepna_pusta(tmp.plansza, 0);

    for (int id = 0; id < ilosc_watkow; id++) {
            threads.push_back(std::thread([&czy_ukonczono, &vec, id, &stk, &plansza_sudoku]() {
                while (!czy_ukonczono) {
                    mtx.lock();
                    if (stk.size()) {
                        int indeks = stk.top().first; //take indeks of "0"
                        Plansza b = stk.top().second; //take sudoku map
                        stk.pop(); //remove first element
                        mtx.unlock();
                        if (b.ile_wszystkich_pustych() == 0) {
                            plansza_sudoku = b;
                            czy_ukonczono = true;
                            break;
                        }
                        wyszukiwanie_wsteczne(stk, b, indeks, glebokosc_rekursji);
                        
                    }
                    else mtx.unlock();
                }
            }));
    }
    for (auto& thread : threads)
        thread.join();
}



int main() {
    glebokosc_rekursji = 25;
    ilosc_watkow = 1;


    Plansza b;

    //przypisanie wszystkich wartości z pliku do tablicy sudoku
    std::ifstream plik("sudoku.txt");
    for (int i = 0; i < rozmiar_sudoku; i++)
    {
        for (int j = 0; j < rozmiar_sudoku; j++) {
            plik >> b.plansza[i][j];
        }
    }
    b.wyswietl_tablice(); //wyświetlenie tablicy


    double startTime = CycleTimer::currentSeconds();
    backtracking_lokalny(b);
    double czas_koncowy = CycleTimer::currentSeconds();


    std::cout << "--------------" << std::endl << "watki: " << ilosc_watkow << std::endl;
    std::cout << "glebokosc: " << glebokosc_rekursji << std::endl;

    b.wyswietl_tablice();
    std::cout  << "czas obliczen =" << czas_koncowy - startTime  << std::endl << std::endl;

    return 0;
}