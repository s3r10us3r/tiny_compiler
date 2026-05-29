# Specyfikacja języka *tiny*

## Spis treści

1. [Wprowadzenie](#1-wprowadzenie)
2. [Leksyka](#2-leksyka)
3. [Typy](#3-typy)
4. [Zmienne](#4-zmienne)
5. [Wyrażenia](#5-wyrażenia)
6. [Instrukcje](#6-instrukcje)
7. [Funkcje](#7-funkcje)
8. [Struktury i klasy](#8-struktury-i-klasy)
9. [Zasięg](#9-zasięg)
10. [Sprawdzanie typów](#10-sprawdzanie-typów)
11. [Gramatyka formalna](#11-gramatyka-formalna)

---

## 1. Wprowadzenie

*tiny* jest statycznie typowanym, kompilowanym językiem programowania ogólnego przeznaczenia. Kompilator przetwarza plik źródłowy w trzech fazach: analizy leksykalnej, analizy składniowej oraz generowania kodu pośredniego LLVM IR, który następnie jest kompilowany przez `clang` do natywnego kodu maszynowego.

Plik źródłowy składa się z dowolnie przeplatanych deklaracji funkcji, deklaracji typów strukturalnych oraz instrukcji tworzących ciało programu głównego. Kolejność deklaracji nie ma znaczenia — kompilator wykonuje wstępną rejestrację wszystkich nazw przed właściwym sprawdzaniem typów i generowaniem kodu.

---

## 2. Leksyka

### 2.1 Komentarze

Komentarze liniowe zaczynają się od `//` i kończą na końcu wiersza. Nie ma komentarzy blokowych.

```
// to jest komentarz
```

### 2.2 Identyfikatory

Identyfikator zaczyna się od litery lub podkreślnika, po którym mogą następować litery, cyfry i podkreślniki.

```
[a-zA-Z_][a-zA-Z_0-9]*
```

Identyfikator nie może być słowem kluczowym ani nazwą wbudowanego typu. Próba użycia zarezerwowanej nazwy jest wykrywana przez parser i kończy się błędem kompilacji.

### 2.3 Słowa kluczowe

Następujące tokeny są zarezerwowane i nie mogą być używane jako nazwy zmiennych, funkcji, struktur, pól ani parametrów:

| Słowo kluczowe | Zastosowanie |
|---|---|
| `let` | deklaracja zmiennej |
| `print` | instrukcja drukowania |
| `read` | instrukcja wczytywania |
| `if` / `else` | instrukcja warunkowa |
| `while` | pętla |
| `break` | przerwanie pętli |
| `fn` | deklaracja funkcji lub metody |
| `return` | instrukcja powrotu |
| `struct` / `class` | deklaracja typu strukturalnego |
| `new` | tworzenie literału struktury |
| `and` / `or` / `xor` / `not` | operatory logiczne |
| `true` / `false` | literały logiczne |

### 2.4 Wbudowane nazwy typów

Nazwy `int`, `float`, `bool`, `str`, `void` są zarezerwowane jako nazwy typów wbudowanych i nie mogą być używane jako identyfikatory.

### 2.5 Literały

| Rodzaj | Składnia | Przykłady |
|---|---|---|
| Całkowity | `[0-9]+` | `0`, `42`, `1000` |
| Zmiennoprzecinkowy | `[0-9]+\.[0-9]+` | `3.14`, `0.5`, `1.0` |
| Logiczny | `true` \| `false` | `true`, `false` |
| Łańcuchowy | `"` `[^"]*` `"` | `"hello"`, `"tiny compiler"` |

### 2.6 Operatory i separatory

```
==  !=  <=  >=  <  >       — porównanie
=                           — przypisanie
+  -  *  /                 — arytmetyczne
->                          — typ zwracany funkcji / metody
.                           — dostęp do pola / wywołanie metody
,                           — separator parametrów i argumentów
:                           — adnotacja typu
;                           — zakończenie instrukcji
( )  [ ]  { }              — nawiasy
```

---

## 3. Typy

### 3.1 Typy proste

| Typ | Opis | Przykład literału |
|---|---|---|
| `int` | 64-bitowa liczba całkowita ze znakiem | `42` |
| `float` | 64-bitowa liczba zmiennoprzecinkowa (IEEE 754) | `3.14` |
| `bool` | wartość logiczna | `true`, `false` |
| `str` | łańcuch znaków | `"hello"` |
| `void` | brak wartości (tylko jako typ zwracany funkcji) | — |

### 3.2 Tablice

Typ tablicowy zapisywany jest jako `typ[rozmiar]`, gdzie rozmiar jest literałem całkowitym. Tablice są jednowymiarowe i alokowane na stosie. Indeksowanie odbywa się operatorem `[]`; indeksy są liczone od zera.

```
let arr: int[10] = 0;     // tablica 10 liczb całkowitych, zainicjowana zerami
arr[3] = 42;
let x: int = arr[3];      // 42
```

Podczas deklaracji tablicy wyrażenie inicjalizujące może być skalarem zgodnego typu — zostanie on zapisany do każdego elementu.

### 3.3 Typy strukturalne

Typ strukturalny jest definiowany słowem kluczowym `struct` lub `class` i może być użyty jako typ zmiennej lub parametru. Wartości strukturalne są przekazywane i przypisywane przez kopię.

---

## 4. Zmienne

### 4.1 Deklaracja

```
let nazwa: typ = wyrażenie;
```

Typ musi być zgodny z typem wyrażenia inicjalizującego. Zmienna jest widoczna od miejsca deklaracji do końca otaczającego bloku. Ponowna deklaracja tej samej nazwy w tym samym zakresie jest błędem.

```
let x: int   = 10;
let y: float = 3.14;
let ok: bool = true;
let msg: str = "witaj";
let buf: int[5] = 0;      // inicjalizacja skalarem — wszystkie elementy = 0
```

### 4.2 Przypisanie

Istniejącej zmiennej, elementowi tablicy lub polu struktury można przypisać nową wartość. Typy muszą być zgodne.

```
x = x + 1;
buf[2] = 99;
punkt.x = 5;
```

---

## 5. Wyrażenia

### 5.1 Priorytety operatorów

Poniższa tabela przedstawia priorytety od najniższego (1) do najwyższego (8).

| Poziom | Operator(y) | Łączność |
|---|---|---|
| 1 | `or` | lewostronna |
| 2 | `xor` | lewostronna |
| 3 | `and` | lewostronna |
| 4 | `==` `!=` `<` `>` `<=` `>=` | lewostronna |
| 5 | `+` `-` | lewostronna |
| 6 | `*` `/` | lewostronna |
| 7 | `-` (jednoargumentowy), `not` | prawostronna |
| 8 | `.pole`, `.metoda(args)`, `[i]` | lewostronna (łańcuch) |

### 5.2 Operatory arytmetyczne

Operatory `+`, `-`, `*`, `/` działają na operandach typu `int` lub `float`. Oba operandy muszą mieć ten sam typ. Dzielenie liczb całkowitych jest całkowite (obcięcie w kierunku zera). Wartości typu `str` nie mogą być operandami żadnego operatora.

### 5.3 Operatory logiczne

`and`, `or`, `xor` wymagają operandów typu `bool` i zwracają `bool`. Operator `not` jest jednoargumentowy i wymaga operanda typu `bool`.

```
let a: bool = true and false;    // false
let b: bool = true or  false;    // true
let c: bool = true xor true;     // false
let d: bool = not false;         // true
```

### 5.4 Operatory porównania

`==`, `!=`, `<`, `>`, `<=`, `>=` porównują dwa operandy tego samego typu numerycznego i zwracają `bool`.

### 5.5 Jednoargumentowy minus

Operator `-` przed wyrażeniem neguje wartość numeryczną (`int` lub `float`).

### 5.6 Łańcuch postfiksowy

Z dowolnego wyrażenia można uzyskać dostęp do pola, wywołać metodę lub zaindeksować tablicę. Operatory te mogą być łączone w dowolną sekwencję.

```
obiekt.pole
obiekt.metoda(arg1, arg2)
obiekt.pole.metoda()
tablica[i]
```

### 5.7 Wywołanie funkcji

```
nazwa(arg1, arg2, ...)
```

Typy i liczba argumentów muszą być zgodne z deklaracją funkcji. Funkcje mogą być wywoływane przed ich deklaracją w pliku źródłowym.

### 5.8 Literał struktury

```
new NazwaTypu { pole1 = wyrażenie1, pole2 = wyrażenie2, ... }
```

Wszystkie pola muszą zostać zainicjowane. Kolejność pól w literale nie musi odpowiadać kolejności w deklaracji, ale każde pole musi wystąpić dokładnie raz.

---

## 6. Instrukcje

Każda instrukcja kończy się średnikiem `;`. Blok instrukcji ujęty w nawiasy klamrowe `{ }` jest traktowany jako pojedyncza instrukcja złożona.

### 6.1 Drukowanie

```
print wyrażenie;
```

Wypisuje wartość wyrażenia na standardowe wyjście, zakończoną znakiem nowej linii. Obsługiwane są wszystkie typy proste. Liczby zmiennoprzecinkowe są drukowane w formacie `printf` z sześcioma miejscami po przecinku. Wartości logiczne są drukowane jako `1` (prawda) lub `0` (fałsz). Łańcuchy znaków są drukowane dosłownie.

### 6.2 Wczytywanie

```
read zmienna;
read tablica[i];
```

Wczytuje wartość ze standardowego wejścia i zapisuje ją do podanej zmiennej lub elementu tablicy. Celem nie może być wyrażenie tablicowe jako całość.

### 6.3 Instrukcja warunkowa

```
if (warunek) {
    ...
};

if (warunek) {
    ...
} else {
    ...
};
```

Warunek musi być wyrażeniem typu `bool`.

### 6.4 Pętla `while`

```
while (warunek) {
    ...
};
```

Warunek musi być wyrażeniem typu `bool`. Pętla wykonuje się tak długo, jak warunek jest prawdziwy.

### 6.5 Przerwanie pętli `break`

```
break;
```

Natychmiast kończy wykonanie najbardziej wewnętrznej pętli `while`. Użycie `break` poza pętlą jest błędem.

### 6.6 Instrukcja `return`

```
return wyrażenie;   // powrót z wartością
return;             // powrót z funkcji void
```

Kończy wykonanie bieżącej funkcji i zwraca wartość do wywołującego. Typ zwracanej wartości musi być zgodny z zadeklarowanym typem zwracanym funkcji. Użycie `return` poza ciałem funkcji jest błędem.

### 6.7 Instrukcja wyrażeniowa

Wywołanie funkcji lub metody może być użyte jako samodzielna instrukcja:

```
obiekt.metoda(arg);
funkcja(arg);
```

---

## 7. Funkcje

### 7.1 Deklaracja

```
fn nazwa(param1: typ1, param2: typ2) -> typ_zwracany {
    ...
};
```

Funkcja może przyjmować dowolną liczbę parametrów (w tym zero). Każdy parametr ma adnotację typu. Typ zwracany następuje po `->`. Ciało funkcji jest blokiem instrukcji.

### 7.2 Typ zwracany `void`

Funkcja zwracająca `void` nie może zawierać instrukcji `return wyrażenie;`. Może zawierać samo `return;` lub po prostu zakończyć się wraz z końcem bloku.

### 7.3 Rekurencja

Funkcje mogą wywoływać same siebie rekurencyjnie.

```
fn fib(n: int) -> int {
    if (n <= 1) { return n; };
    return fib(n - 1) + fib(n - 2);
};
```

### 7.4 Wyprzedzające odwołania

Funkcje i typy strukturalne mogą być używane przed ich deklaracją w pliku źródłowym. Kompilator rejestruje wszystkie sygnatury w pierwszym przebiegu, zanim przystąpi do sprawdzania typów i generowania kodu.

```
print suma(10);        // wywołanie przed deklaracją

fn suma(n: int) -> int {
    if (n <= 0) { return 0; };
    return n + suma(n - 1);
};
```

### 7.5 Zasięg parametrów

Parametry są widoczne wewnątrz ciała funkcji. Zmienne lokalne zadeklarowane w ciele przesłaniają parametry o tej samej nazwie.

---

## 8. Struktury i klasy

Słowa kluczowe `struct` i `class` są synonimami — generują identyczny kod. Różnią się jedynie semantycznie dla czytelności.

### 8.1 Deklaracja

```
struct NazwaTypu {
    pole1: typ1;
    pole2: typ2;
    ...
    fn metoda1(self, param: typ) -> typ_zwracany {
        ...
    };
};
```

Pola i metody mogą być deklarowane w dowolnej kolejności wewnątrz bloku struktury. Każde pole kończy się `;`. Każda metoda kończy się `};`.

### 8.2 Parametr `self`

Pierwszy parametr metody może być nazwany `self` bez adnotacji typu — kompilator automatycznie nadaje mu typ wskaźnikowy na otaczającą strukturę. Dzięki temu mutacje pól przez `self` są widoczne po stronie wywołującego.

```
struct Licznik {
    n: int;

    fn zwieksz(self) -> void {
        self.n = self.n + 1;    // zmiana persystuje u wywołującego
    };

    fn wartosc(self) -> int {
        return self.n;
    };
};
```

### 8.3 Wywołanie metody

```
let c: Licznik = new Licznik { n = 0 };
c.zwieksz();
print c.wartosc();   // 1
```

### 8.4 Semantyka kopiowania wartości

Przypisanie zmiennej strukturalnej lub użycie jej jako wartości pola w literale struktury tworzy pełną kopię wartości w chwili przypisania. Późniejsze mutacje oryginału nie wpływają na kopię.

```
let a: Pkt = new Pkt { x = 1, y = 2 };
let b: Pkt = a;        // b jest kopią a

a.x = 99;
print b.x;             // nadal 1
```

Metody mutujące przez `self` są wyjątkiem: `self` jest wskaźnikiem, więc zmiany dokonane wewnątrz metody są widoczne u wywołującego.

### 8.5 Zagnieżdżone struktury

Pole struktury może mieć typ innej struktury. Dostęp do zagnieżdżonych pól i metod odbywa się przez łańcuch operatora `.`.

```
struct Odcinek {
    a: Pkt;
    b: Pkt;
};

let seg: Odcinek = new Odcinek { a = p1, b = p2 };
print seg.a.x;
```

### 8.6 Wykrywanie duplikatów

Kompilator wykrywa i zgłasza błąd przy:
- zduplikowanej nazwie struktury lub klasy,
- zduplikowanej nazwie funkcji,
- zduplikowanym polu w obrębie jednej struktury,
- zduplikowanej metodzie w obrębie jednej struktury.

---

## 9. Zasięg

Język stosuje statyczne zasięgi blokowe.

- Każdy blok `{ }` tworzy nowy zasięg. Zmienne zadeklarowane wewnątrz bloku nie są widoczne poza nim.
- Parametry funkcji tworzą zasięg otaczający zasięg ciała funkcji.
- Zmienne lokalne przesłaniają zewnętrzne zmienne o tej samej nazwie.
- Deklaracje funkcji i struktur są widoczne w całym pliku źródłowym niezależnie od miejsca deklaracji.

---

## 10. Sprawdzanie typów

Sprawdzanie typów odbywa się po analizie składniowej. Kompilator zbiera wszystkie błędy semantyczne w jednym przebiegu i wypisuje je razem, nie przerywając analizy przy pierwszym błędzie.

### 10.1 Wykrywane błędy semantyczne

| Kategoria | Przykład |
|---|---|
| **Zmienne** | |
| Niezgodność typów w deklaracji | `let x: int = 3.14;` |
| Niezgodność typów w przypisaniu | `let y: float = 1.0; y = 5;` |
| Typ `void` jako typ zmiennej | `let z: void = 0;` |
| Niezadeklarowana zmienna | `let r: int = ghost;` |
| Ponowna deklaracja w tym samym zasięgu | `let x: int = 1; let x: int = 2;` |
| **Wyrażenia** | |
| Operator `not` na nie-`bool` | `not 5` |
| Jednoargumentowy minus na nie-numerycznym | `-true` |
| Niezgodne typy w wyrażeniu binarnym | `5 == 3.14` |
| Operacje arytmetyczne na `bool` | `true + false` |
| Operatory logiczne na nie-`bool` | `1 and 2` |
| Użycie łańcucha jako operandu operatora | `"a" + "b"` |
| **Tablice** | |
| Indeksowanie nie-tablicy | `let x: int = 5; x[0]` |
| Indeks tablicy nie jest `int` | `arr[3.14]` |
| **Dostęp do pól** | |
| Dostęp do pola nie-struktury | `let n: int = 5; n.x` |
| Nieznana nazwa struktury (dostęp do pola) | zmienna o typie niezdefiniowanej struktury |
| Nieistniejące pole struktury | `punkt.z` gdy `Punkt` ma tylko `x` i `y` |
| **Funkcje** | |
| Niezdefiniowana funkcja | `foo()` bez deklaracji `fn foo` |
| Zła liczba argumentów funkcji | `add(1)` gdy `fn add(a: int, b: int)` |
| Zły typ argumentu funkcji | `add(1, 3.14)` gdy oba parametry to `int` |
| Brak wartości zwracanej w funkcji innej niż `void` | samo `return;` w `fn f() -> int` |
| Niezgodność typu zwracanego | `return 3.14;` w funkcji `-> int` |
| Funkcja `void` zwraca wartość | `return 42;` w funkcji `-> void` |
| Wywołanie `return` poza funkcją | `return 0;` na poziomie programu |
| **Metody** | |
| Wywołanie metody na nie-strukturze | `let n: int = 5; n.foo()` |
| Wywołanie metody na strukturze bez metod | `plain.foo()` gdy typ nie ma żadnej metody |
| Nieistniejąca metoda | `obiekt.nieistnieje()` |
| Zła liczba argumentów metody | `c.set()` gdy `fn set(self, v: int)` |
| Zły typ argumentu metody | `c.set(3.14)` gdy parametr to `int` |
| **Literały struktur** | |
| Nieznany typ struktury w literale | `new Nieznany { x = 1 }` |
| Brak pola w literale struktury | pominięcie wymaganego pola |
| Niezgodność typu pola w literale struktury | `new Pkt { x = 3.14 }` gdy `x: int` |
| **Instrukcja `read`** | |
| Wczytywanie bezpośrednio do tablicy | `read arr` gdzie `arr: int[5]` |
| Cel `read` nie jest zmienną ani elementem tablicy | `read 5` |
| **Przepływ sterowania** | |
| Warunek `if` nie jest `bool` | `if (5)` |
| Warunek `while` nie jest `bool` | `while (n)` gdzie `n: int` |
| `break` poza pętlą | `break;` na poziomie programu |
| **Duplikaty** | |
| Zduplikowana nazwa struktury lub klasy | dwie deklaracje `struct Point` |
| Zduplikowana nazwa funkcji | dwie deklaracje `fn add` |
| Zduplikowane pole w strukturze | `x: int; x: float;` w jednej strukturze |
| Zduplikowana metoda w strukturze | dwie deklaracje `fn get(self)` |
| Pole struktury o nieznanym typie strukturalnym | `a: NieznanyTyp;` w deklaracji pola |

### 10.2 Błędy parsera

Błędy wykryte podczas analizy składniowej kończą kompilację natychmiast po pierwszym naruszeniu. Dotyczy to m.in.:

- użycia słowa kluczowego jako nazwy (`fn while(...)`),
- użycia nazwy wbudowanego typu jako identyfikatora (`struct int { }`),
- użycia literału logicznego jako nazwy (`struct true { }`).

---

## 11. Gramatyka formalna

Gramatyka zapisana jest w notacji EBNF, gdzie:
- `*` oznacza zero lub więcej powtórzeń,
- `?` oznacza element opcjonalny,
- `|` oznacza alternatywę,
- `( )` grupuje elementy.

Symbole zapisane WERSALIKAMI to terminale. Symbole pisane małymi literami to nieterminale.

### 11.1 Symbole terminalne

```
LET       = "let"
READ      = "read"
PRINT     = "print"
IF        = "if"
ELSE      = "else"
WHILE     = "while"
BREAK     = "break"
FN        = "fn"
RETURN    = "return"
STRUCT    = "struct"
CLASS     = "class"
NEW       = "new"
AND       = "and"
OR        = "or"
XOR       = "xor"
NOT       = "not"
BUILT_IN  = "int" | "float" | "str" | "bool" | "void"
BOOL_LIT  = "true" | "false"
ID        = [a-zA-Z_][a-zA-Z_0-9]*
INT_LIT   = [0-9]+
FLOAT_LIT = [0-9]+\.[0-9]+
STR_LIT   = '"' [^"]* '"'
COMP_EQ   = "=="
NEQ       = "!="
LEQ       = "<="
GEQ       = ">="
ARROW     = "->"
LT        = "<"
GT        = ">"
EQ        = "="
PLUS      = "+"
MINUS     = "-"
TIMES     = "*"
DIV       = "/"
DOT       = "."
COMMA     = ","
LB        = "("
RB        = ")"
LS        = "["
RS        = "]"
LC        = "{"
RC        = "}"
SC        = ";"
CL        = ":"
EOF       = koniec pliku
```

### 11.2 Symbole nieterminalne

```
prog => stmt* EOF

stmt => var_decl SC
      | print_stmt SC
      | read_stmt SC
      | if_stmt SC
      | while_stmt SC
      | BREAK SC
      | return_stmt SC
      | func_decl SC
      | struct_decl SC
      | assgn_or_call SC

block => LC stmt* RC

type => BUILT_IN ( LS INT_LIT RS )*
      | ID

var_decl    => LET ID CL type EQ expr
print_stmt  => PRINT expr
read_stmt   => READ postfix
if_stmt     => IF LB expr RB block ( ELSE block )?
while_stmt  => WHILE LB expr RB block
return_stmt => RETURN expr?

assgn_or_call => postfix ( EQ expr )?

func_decl  => FN ID LB param_list RB ARROW type block
param_list => ( param ( COMMA param )* )?
param      => ID CL type

struct_decl   => ( STRUCT | CLASS ) ID LC struct_member* RC
struct_member => field_decl | method_decl
field_decl    => ID CL type SC
method_decl   => FN ID LB method_params RB ARROW type block SC

method_params => "self" ( COMMA param )*
               | param_list

struct_lit      => NEW ID LC field_init_list? RC
field_init_list => field_init ( COMMA field_init )*
field_init      => ID EQ expr

arg_list => ( expr ( COMMA expr )* )?

expr        => logical_xor ( OR  logical_xor )*
logical_xor => logical_and ( XOR logical_and )*
logical_and => relational  ( AND relational  )*
relational  => math_expr ( ( COMP_EQ | NEQ | LT | GT | LEQ | GEQ ) math_expr )*
math_expr   => term  ( ( PLUS  | MINUS ) term  )*
term        => unary ( ( TIMES | DIV   ) unary )*
unary       => MINUS unary
             | NOT   unary
             | postfix

postfix => primary suffix*
suffix  => DOT ID LB arg_list RB
         | DOT ID
         | LS expr RS

primary => INT_LIT
         | FLOAT_LIT
         | STR_LIT
         | BOOL_LIT
         | ID LB arg_list RB
         | struct_lit
         | LB expr RB
         | ID
```
