# Živý obraz - firmware

Vítejte v repozitáři projektu Živý obraz s firmwarem pro vývojové desky založené na ESP32. Živý obraz slouží pro krmení ePaper/e-Ink displejů obrazovými daty z webového serveru ať už jde o bitmapu nebo vlastní formát.

  * Základní informace najdete na webu projektu: https://zivyobraz.eu/
  * Konkrétní informace ohledně zprovoznění jsou v dokumentaci na adrese: https://wiki.zivyobraz.eu/

**V dokumentaci také naleznete předkompilovaný firmware k flashnutí pro několik nejběžnějších typů ePaperů.**
**Výchozí heslo pro Wi-Fi, kterou po nahrátí FW deska vysílá, je: _zivyobraz_**

----

Ve zkratce k vlastní kompilaci a nastavení v kódu firmwaru:

Budete potřebovat mít nainstalované následující knihovny:
> zinggjm/GxEPD2@^1.5.2  
> adafruit/Adafruit GFX Library@^1.11.5  
> madhephaestus/ESP32AnalogRead@^0.2.1  
> adafruit/Adafruit SHT4x Library@^1.0.2 

V kódu _**main.cpp**_ nezapomeňte odkomentovat:
1. Typ použité desky (ESPink, ES3ink, REMAP_SPI ...)
2. Případné čidlo SHT40 připojené přes uŠup pro vyčítání teploty a vlhkosti a zasílání obojího na server:
> //#define SHT40
3. Typ displeje. V případě GRAYSCALE musíte z platformio.ini vyhodit "zinggjm/GxEPD2" (stačí zakomentovat), jinak dojde ke kolizi knihoven a kód nebude funkční. V tom případě se využije "lib/GxEPD2_4G". Pro ostatní displeje (BW, 3C, 7C) nechte zinggjm/GxEPD2 aktivní, s 4G verzí nic dělat nemusíte.
> #define TYPE_BW // black and white  
> //#define TYPE_3C // 3 colors - black, white and red/yellow  
> //#define TYPE_GRAYSCALE // grayscale - 4 colors  
> //#define TYPE_7C // 7 colors
4. Odkomentujte definici konkrétního ePaperu, který zprovozňujete. Od řádku 86 začíná tato sekce a je potřeba zvolit konkrétní displej, např.:
> // BW  
> //#define D_GDEY0213B7    // 122x250, 2.13"  
> //#define D_GDEW042T2     // 400x300, 4.2"  
> #define D_GDEW075T7     // 800x480, 7.5"  

Po úspěšní kompilaci a flashnutí desky pokračujte v dokumentaci "Oživení vlastního ePaperu":  
https://wiki.zivyobraz.eu/doku.php?id=start#oziveni_vlastniho_epaperu