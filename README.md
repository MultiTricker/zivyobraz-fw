# Živý obraz - firmware

Vítejte v repozitáři projektu Živý obraz s firmwarem pro vývojové desky založené na ESP32. Živý obraz slouží pro krmení ePaper/e-Ink displejů obrázky ze serveru.

Základní informace najdete na webu projektu: https://zivyobraz.eu/

Konkrétní informace ohledně zprovoznění poté v dokumentaci na adrese: https://wiki.zivyobraz.eu/

Zrychleně ke kompilaci a nastavení v kódu firmwaru:

Budete potřebovat mít nainstalované následující knihovny:
> zinggjm/GxEPD2@^1.5.2  
> adafruit/Adafruit GFX Library@^1.11.5  
> madhephaestus/ESP32AnalogRead@^0.2.1  
> adafruit/Adafruit SHT4x Library@^1.0.2 

V kódu _**main.cpp**_ nezapomeňte odkomentovat:
1. Případné čzdlo SHT40 připojené přes uŠup pro vyčítání teploty a vlhkosti a zasílání obojího na server:
> //#define SHT40
2. Typ displeje. V případě GRAYSCALE musíte z platformio.ini vyhodit "zinggjm/GxEPD2" (stačí zakomentovat), jinak dojde ke kolizi knihoven a kód nebude funkční. V tom případě se využije "lib/GxEPD2_4G". Pro ostatní displeje (BW, 3C, 7C) nechte zinggjm/GxEPD2 aktivní, s 4G verzí nic dělat nemusíte.
> #define TYPE_BW // black and white  
> //#define TYPE_3C // 3 colors - black, white and red/yellow  
> //#define TYPE_GRAYSCALE // grayscale - 4 colors  
> //#define TYPE_7C // 7 colors
3. Odkomentujte přímo konkrétní ePaper, který máte připojený. Od řádku 86 začíná tato sekce a je potřeba zvolit konkrétní displej, např.:
> // 7.5" b/w 800x480  
> GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT> display(GxEPD2_750_T7(/*CS*/ PIN_SS, /*DC*/ PIN_DC, /*RST*/ PIN_RST, /*BUSY*/ PIN_BUSY));

Po úspěšní kompilaci a flashnutí desky pokračujte v dokumentaci "Oživení vlastního ePaperu":  
https://wiki.zivyobraz.eu/doku.php?id=start#oziveni_vlastniho_epaperu