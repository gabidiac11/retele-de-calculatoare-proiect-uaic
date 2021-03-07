# proiect-retele-calculatoare-c
### QuizzGame (B) [Propunere Continental]

  Implementati un server multithreading care suporta oricati clienti. Serverul va coordona clientii care raspund la un set de intrebari pe rand, in ordinea in care s-au inregistrat. Fiecarui client i se pune o intrebare si are un numar n de secunde pentru a raspunde la intrebare. Serverul verifica raspunsul dat de client si daca este corect va retine punctajul pentru acel client. De asemenea, serverul sincronizeaza toti clienti intre ei si ofera fiecaruia un timp de n secunde pentru a raspunde. Comunicarea intre server si client se va realiza folosind socket-uri. Toata logica va fi realizata in server, clientul doar raspunde la intrebari. Intrebarile cu variantele de raspuns vor fi stocate fie in fisiere XML fie intr-o baza de date SQLite. Serverul va gestiona situatiile in care unul din participanti paraseste jocul astfel incat jocul sa continue fara probleme.       

 ### Indicatii: 

  Actori: serverul, clientii ( un numar nelimitat) Activitati: inregistrarea clientilor incarcarea de intrebari din formatul mentionat anterior de catre server intrebare adresata catre clienti in ordinea inregistrarii lor, clientul va alege din optiunile oferite intr-un numar de secunde n parasirea unui client va face ca el sa fie eliminat din rundele de intrebari terminarea jocului cand intrebarile au fost parcurse -> anuntare castigator catre toti clientii.

### Instalre biblioteca grafica ncurses:
```
sudo apt-get install libncurses5-dev libncursesw5-dev
```

### Compilare client.c integrata cu libraria grafica:
```
 gcc client.c -I/usr/include/libxml2 -lxml2 -o client -lncurses
```

### Compilare server
```
  gcc server.c -o server
  
 ```
 
 ```
  sudo apt-get install build-essential gnome-devel
 ```
### Folosire:
```
1. Dupa ce conexiunea la server s-a realizat, pe ecran va va fi indicat sa introduceti un username si un enter (username-ul poate fi lasat necompletat si veti prelua un nume autogenerat)
2. Mai departe, daca sunteti primul care v-ati conectat este necesar ca cel putin inca un client sa se alature (lucru ce va aparea explicat pe ecran)
3. Dupa ce sunt suficienti utilizatori, in cazul in care va aparea corpul intrebarii si 4 variante de raspuns, navigati cu sagetile si apasati enter pentru a alege o varianta
4. Dupa ce ati selectat o varianta de raspuns, veti fi instiintat daca ati raspuns corect, dupa care daca nu ati ramas singur in sesiune va aparea numele utilizatorului care raspunde la intrebare in acest timp, ceea ce inseamna sa asteptati pana cand timpul ii va expira acestuia sau va trimite un raspuns si in cele din urma va va veni randul (dupa ce si alti utilizatori au primit si raspuns la intrebarea curenta)
5. La final vor aparea rezultatul dumneavoastra, un mesaj daca ati castigat sau numele utilizatorului care a castigat precum si scorul sau.
```
