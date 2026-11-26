# Zadání vlastníka: simulace světa

Původní znění, jak ho vlastník vložil na začátku práce (včetně úvodní věty).

---

Skoc si do vetve a implementuj mi realisticky simulacni engine na krajinotvorbu s prilozenym zadanim: optimalizuj, vykon je dulezity, ale kriticky je vysledny vzhled

DeRivean — zadání pro nezávislou implementaci simulace světa
Navrhni a implementuj deterministickou simulaci, která přímo pohání hratelný svět desktopové voxelové hry DeRivean. Hráč se může procházet, létat, kopat, stavět, přesměrovat vodu a cestovat v čase. Svět má být přesvědčivý z úrovně člověka i při přeletu několika kilometrů. Hlavní prioritou je tvar krajiny, jeho měřítko a geologická historie. Porost, fauna a grafické efekty počkají.
Máš volnost zvolit vlastní algoritmy, prostorové členění i numerické metody. Referenční implementace má známé vady a reprodukce jejích výsledků není cílem. Chceme porovnat, jak si se stejným zadáním poradí nezávislý návrh.
1. Co má hráč zažít
Hráč vyrazí od pobřeží do vnitrozemí. Může přejít nížinu, vystoupat na první hřbet, zase sejít do širokého údolí a až za ním uvidět výrazné pohoří. Jinde dojde k okraji zdánlivě rovné plošiny a pod sebou objeví soutěsku. V pískovcové oblasti se může protáhnout mezi věžemi nebo proletět pod skalní branou. Z protější strany údolí má poznat, že jeden svah tvoří pevná skála a na druhém se pod stěnou nahromadila suť.
Stejně důležitá jsou obyčejná místa. Několik kilometrů klidné nížiny je žádoucí. Jeden mimořádný skalní útvar v krajině funguje. Třicet podobných útvarů ve stejném rozestupu působí jako chyba generátoru.
Kritériem je vysvětlitelná krajina. Vybereš-li na mapě kopec, údolí, balvan nebo nános písku, mělo by jít zjistit, co tam původně bylo, které procesy tam působily, odkud se vzal dnešní materiál a kam se poděla odnesená část.
2. Prostor, čas a determinismus
Svět je horizontálně prakticky neomezený a plochý. Používej skutečné prostorové jednotky. y = 0 m je referenční oceánský datum. Skutečnou hladinu určuje množství vody a tvar pánve. V žhavém světě nemusí kapalný oceán vůbec existovat.
Výška i hloubka jsou herně otevřené. Pod povrchem má být místo na celé podzemní království. Tři minuty kopání nesmějí hráče dostat na konec světa. Hlubší vrstvy mohou být oproti Zemi prostorově komprimované. Pokud hráč opravdu propojí vodu s prázdnotou za hranicí světa, voda může uniknout.
Čas nula představuje žhavé počáteční těleso. Ochlazování, tuhnutí, vznik kůry a kondenzace vody mají předcházet procesům, které na nich závisejí. Před vstupem do světa musí jít zvolit seed a stáří. Mezi kontrolní stáří patří 0, 10, 100 a 500 milionů let, 1 a 2 miliardy let a 4,6 miliardy let.
Rozlišuj letopočet a kalendář. Letopočet určuje okamžik geologické historie. Kalendář je běžný herní běh, během nějž se pohybuje voda, vane vítr, prší a průběžně působí i pomalé procesy. Za normální herní večer hráč neuvidí vyrůst kontinent. Události jako sesuv, povodeň nebo průlom hráze mohou být rychlé.
Čas posouvá vnější autorita. Musí jít simulaci zastavit, změnit rychlost, skočit na jiné datum nebo stát na jednom místě a pozorovat zrychlený vývoj. Oba způsoby postupu mají vést ke slučitelnému stavu.
Nedotčený svět určuje verze modelu, seed, souřadnice a čas. U obydleného světa patří ke vstupům také historie zásahů do zvoleného data. Výsledek nesmí záviset na pořadí načtení chunků, počtu vláken, poloze kamery ani na tom, zda už danou oblast někdo navštívil.
3. Co znamená simulovat historii
Samotný parametr age u funkce generující výšku nestačí. U procesu, který tvrdíš, že simuluješ, určuj předchozí stav, působící síly nebo toky, časový krok a nový stav. Například během eroze ubude konkrétní hornina, částice se dostanou do transportu, část se uloží níž a nový povrch ovlivní příští odtok.
Celé miliardy let nemusíš počítat po sekundách. Použij vhodné kombinace hrubé regionální historie, adaptivních kroků, checkpointů, analytických řešení a jemného lokálního výpočtu. U každého subsystému výslovně uveď, co je numericky vyvíjený stav, co analytická aproximace, co seedovaná počáteční podmínka a co zatím chybí. Přesnost posuzuj podle dopadu na hru, reprodukovatelnosti a bilancí.
Výšková mapa může sloužit jako odhad povrchu, vstup pro povodí nebo vzdálená projekce. Autoritativní dotaz ale musí umět rozlišit například skálu nahoře, vzduch pod ní a další skálu pod nohama na stejném X/Z. Šum může určit počáteční nehomogenitu, klimatickou oblast nebo porušení horniny. Samotné přidávání šumu neřeší chybějící fyzikální mechanismus.
4. Kůra, kontinenty a pohoří
Modeluj kontinenty, oceánské pánve, ostrovy i souostroví. Tvar pobřeží, hranice tektonické desky a hranice výpočetního regionu jsou tři různé věci. Kontinent může obsahovat staré horské pásmo, vnitrozemskou pánev i aktivní rift. Cesta od pobřeží nesmí automaticky znamenat stálé stoupání.
Tektonické oblasti potřebují směr a rychlost pohybu. Z relativního pohybu sousedů odvoď kompresi, extenzi a smyk. Šikmá kolize může současně stlačovat i posouvat horniny podél rozhraní. Aktivní hranice má prostorovou šířku a proměnlivou intenzitu. Může se ohýbat, větvit nebo přecházet mezi úzkým zlomem a širokým pásem deformace. Na mapě široké 128 km typicky nemá být mozaika desítek stejně velkých desek.
Komprese vytváří podmínky pro zdvih. O výsledném tvaru rozhodují také dostupný materiál, horninové vrstvy, historie deformace, stabilita svahů, klima a odnos. Extenze má umět vytvořit sníženinu nebo zřetelný zlom i v jinak obyčejném terénu. Vulkanismus a přísun magmatu potřebují zdroj a vazbu na geologické podmínky.
Výšku hlídá dostupná hmota. Jako výchozí herní představu lze uvažovat zásobu za oblast 1 km² a například 2 km³ mobilizovatelné horniny. Poměr 2:1 je laditelný návrh. Materiál může přicestovat ze sousedství a soustředit se, takže mimořádně vysoké pohoří je legitimní. Trvalý tlak po vyčerpání zásoby nesmí za miliardu let vyrobit dvousetkilometrovou horu. Stejně tak hydraulické účetnictví nesmí potřebovat kolem každého účetního regionu několikakilometrovou skalní zeď.
5. Materiály, eroze a tvar reliéfu
Každý materiál má vlastní typovanou definici v samostatném souboru. Přidání horniny nemá vyžadovat rozšiřování jedné centrální tabulky všech fyzikálních hodnot. Svět skládáme z herně smysluplných materiálů, není nutné sestavovat každý kámen z chemických prvků.
Materiálový slovník musí odlišit alespoň pevné krystalické horniny, vrstevnatý pískovec, slabší sedimentární horniny, rozpustné karbonáty, volné hrubé úlomky, štěrk, písek, prach, jíl, vodu a taveninu. Pro jednotlivé procesy používej příslušné vlastnosti. Patří mezi ně hustota, pevnost, soudržnost, odolnost vůči abrazi, vrstevnatost a pukliny, rozpustnost, velikost částic, pórovitost a propustnost. Tepelný vývoj potřebuje vodivost, tepelnou kapacitu a fázové chování. Pórovitost určuje dostupný prostor pro vodu, propustnost rychlost jejího pohybu. Tyto dvě veličiny musí mít samostatný účinek.
V krajině se mají potkávat následující procesy:
Proces
Co má být vidět v terénu
Selektivní zvětrávání
Odolná vrstva vystupuje ze svahu. Slabší podloží ustupuje, takže mohou vznikat římsy, převisy a izolované věže.
Říční eroze a sedimentace
Voda se sbírá v povodí, přítoky se spojují, koryto se zařezává a při poklesu transportní schopnosti vznikají náplavy.
Svahový transport
Z pevné stěny se může oddělit blok, spadnout, cestovat se sesuvem, zastavit se ve svahu nebo až v údolí a dál zvětrávat.
Ledovcová modelace
Kde klima, sněhová akumulace a topografie dovolí pohyb ledu, mohou vzniknout široké trogy, strmé karové stěny a přehloubené nebo morénou hrazené pánve.
Vítr a mráz
Obnažená skála reaguje na expozici, mráz a dostupné abrazivo. Vítr má mít směr a sílu používané stejnou simulací, kterou ukazuje inspector.
Podzemní voda
Voda proniká puklinami a propustnými vrstvami, může se vracet v pramenech. Ve vhodném podloží může vytvořit podzemní prostor a propojené jeskyně.
Písek není univerzální výstup. Zvětráváním mohou vznikat balvany, suť, štěrk, písek, prach, jíl i rozpuštěné látky. Zdrojová hornina určuje jejich poměry a transport je dále třídí. Bez mechanismu odnosu má materiál zůstat poblíž zdroje. Balvan má mít původ v konkrétní skalní hmotě a její materiál si při přesunu zachovat.
Ložiska mají vycházet z podmínek vzniku tam, kde to hra umí zachytit. U diamantů využij tlak, teplotu a geologickou historii, u dalších surovin například hostitelskou horninu, vrstvy nebo žíly. U fosilních zdrojů je přijatelný seedovaný výskyt ve vhodné geologické pasti. Simulace života teď není součástí zadání.
6. Voda, atmosféra a zásahy hráče
Voda je jeden materiál. Řeku, jezero a oceán odlišuje prostředí, propojení, proudění a složení vody, zejména obsah soli. Při mísení sleduj objem vody i množství rozpuštěné soli. Vítr může řídit vlnění a kalendář může řídit příliv prostřednictvím lunárního cyklu. Oba jevy přemisťují existující vodu.
Déšť dopadá na konkrétní podklad. Část vody se vsákne podle vlastností a aktuálního nasycení, část odteče a část se může zadržet. Stálé jezero může ovlivnit okolní podzemní vodu. V plochém světě si zvol deterministickou náhradu zeměpisné šířky. Místní teplota, vláha, vítr, nadmořská výška a dostupnost vody mají společně vytvářet podmínky pro budoucí biomy.
Také oceán je konečný. Když hráč otevře průchod ze dna do voidu, zásoba musí klesat a při vhodném umístění otvoru může odtéct celý propojený oceán. Únik musí být v bilanci. Až zmizí vodní plocha, mají se změnit podmínky, které na ní závisely, například dostupnost vody pro výpar a vlhkost okolí.
Simulace přijímá změny zpět ze hry. Prokopání sedla může spojit dvě povodí. Voda z horního jezera nateče do spodního, spodní může dosáhnout přelivné hrany a zaplavit třetí oblast. Přehrazení toku může zatopit okolí a přesunout sedimentaci jinam. Po skoku o sto let musí být důsledky zásahu zjistitelné. Architektura má obdobně umožnit pozdější požár lesa a jeho vliv na atmosféru, přestože vegetaci teď neřešíme.
7. Skutečné krajiny jako vizuální a geologické reference
Vyhledej si fotografie z úrovně člověka, pohledy přes údolí i letecké snímky. Pro každý typ si zapiš přibližné měřítko, charakter siluety, příčný profil a mechanismus vzniku. Fotografie slouží také k posouzení četnosti prvků. Prachovské věže patří do konkrétní geologické oblasti, nemají být výchozím tvarem všech kopců.
Reference
Co z ní převzít jako pozorovatelný požadavek
Adršpašsko-teplické a Prachovské skály, Česko
Původně souvislý pískovcový masiv se rozpadá podél puklin a vrstevních spár. Na malé vzdálenosti se střídají mohutné bloky, úzké průchody, zaoblené vršky, svislé stěny i zřícené balvany. Studuj zejména poměr mezi šířkou průchodu a výškou stěn. [AOPK – Adršpašsko-teplické skály](https://portal.nature.cz/cs/web/cz/w/uzemi-2186) · [Fotogalerie Prachovských skal](https://www.prachovskeskaly.cz/en/gallery)
Pravčická brána, České Švýcarsko
Zkouška skutečně objemového světa. Skalní stěna může lokálně ztratit materiál tak, že zůstane nosný oblouk, otvor průchozí z obou stran a dva přirozené pilíře. Stejnou geometrii musí znát obraz, kolize i dotaz na materiál. [Správa NP – Pravčická brána](https://www.npcs.cz/pravcicka-brana) · [Geologie oblasti](https://www.npcs.cz/geologie)
Krkonoše, Sněžka, Obří důl a Labský důl
Vedle sebe existují staré náhorní povrchy, široká údolní dna, strmé stěny karů, suť i výrazný jednotlivý vrchol. Ledovec přemodeloval některá dřívější říční údolí do širokého příčného profilu U. Okolní hřbety si mohou zachovat úplně jiný charakter. [KRNAP – Reliéf a jeho tvary](https://www.krnap.cz/priroda/fenomeny/relief-a-jeho-tvary/)
Jizerské a Krušné hory
Srovnávací reference pro klidnější náhorní partie, lokální skalní výchozy a rozdíl mezi dlouhým pozvolným přístupem a výraznou hranou pohoří. Zajímá nás i krajina, kde nejvýraznější skála vyčnívá z jinak mírného povrchu.
Grand Teton a Jenny Lake, USA
Výrazné horské čelo vedle nízké pánve. Od jezera lze na jediném pohledu přečíst hladinu, ústí údolí, několik různých svahů a vzdálený hřeben. Studuj vazbu tektonického zdvihu, ledovcové eroze a morén. [NPS – Geologie Grand Tetonu](https://www.nps.gov/grte/learn/nature/geology.htm) · [Pohled přes Jenny Lake](https://www.nps.gov/places/000/jenny-lake-overlook.htm)
Grand Canyon of the Yellowstone, USA
Řeka se zařezává do různě odolné vulkanické horniny. Hydrotermálně přeměněné horniny podléhají erozi jinak než odolnější úseky. Sleduj profil V, změny sklonu stěn a lokální skalní stupně. [NPS – Grand Canyon of the Yellowstone](https://www.nps.gov/yell/learn/nature/grand-canyon.htm)
Pro studium numerických metod jsou relevantní Landlab a jeho model SPACE pro společný vývoj eroze podloží, transportu a ukládání sedimentu. Dalšími vodítky jsou Badlands a goSPL pro dlouhodobý vývoj reliéfu. Výběr konkrétní knihovny je otevřený. [Publikace autorů modelu SPACE](https://gmd.copernicus.org/articles/10/4577/2017/)
8. Architektura a běh hry
@simulation vlastní přírodní svět. Má data, fyzikální vlastnosti, historii a prostorové dotazy. Musí být použitelná bez voxelového enginu, rendereru i Electronu. Stejná data má jít zkoumat v testu nebo alternativním prohlížeči, případně přes WASM.
@engine vlastní hratelnou reprezentaci. Převádí svět na voxely, drží hráčské úpravy a kolize, předává zásahy simulaci a připravuje data pro zobrazení. C++ je autorita pro herní stav. Electron řeší platformu, Three.js vykreslení.
Drahé výpočty nesmějí blokovat pohyb, kameru ani vykreslování. Použij nativní CPU vlákna tam, kde to pomůže. Hráč může proletět do prostoru, který se ještě dopočítává. Když se po načtení výjimečně ocitne uvnitř pevné hmoty, vyřeš překryv a přesuň ho nad povrch při zachování vodorovné pozice. Jeskyně a průchozí skalní brány musí zůstat použitelné.
Pro průzkum dej hráči přepnutí letu dvojím stiskem mezerníku a výrazné zrychlení přes Ctrl. Okolí má být čitelné i ze vzduchu. Oproti původnímu malému výřezu je žádoucí přibližně dvojnásobný dosah. Detail ubírej dost pozdě, aby šlo rozeznat skutečný tvar údolí a hor. Přechod úrovní detailu nesmí kreslit umělé štíty, schody nebo trhliny.
Rozděl zdrojáky podle domén a odpovědností. Produkční soubory drž přibližně do 250 řádků. Materiály mají samostatné definice. Pro integraci do existujícího repozitáře používej modulové mise tasky, xmake a nezávislé review podstatných změn.
Mlhu a osvětlení řeš pouze v rozsahu nutném pro průzkum. Při hodnocení musí být dobře čitelná samotná geometrie. Faunu a dekorativní rozmisťování objektů vynech.
9. Inspector a diagnostika
Umožni vybrat bod na mapě, vypínat jednotlivé vrstvy a zobrazit stejné místo v různém stáří. Hráč i vývojář mají být schopni rozlišit chybu simulace od chyby převodu na voxely nebo vzdálenou síť.
Inspector má zpřístupnit původní výšku podloží a výsledný povrch, stáří kůry, litologii, hustotu puklin, zásobu materiálu, mocnost a složení sedimentu, množství či rychlost zdvihu, zvětrávání, eroze a ukládání. U desek zobraz identitu, směr pohybu, relativní pohyb a intenzitu komprese, extenze a smyku. U atmosféry a vody zobraz vítr, teplotu, srážky, vsak, nasycení a odtok. U naměřené hodnoty rozliš okamžitou rychlost od celkového množství od začátku vývoje.
Důležitý praktický dotaz zní: „Proč je právě tady písek?“ Odpověď může být zvětrávající pískovec, říční náplav, pláž nebo chyba materiálu v rendereru. Inspector má tyto případy pomoci odlišit.
10. Známé vady a reprodukční případy
V dosavadním experimentu některé pohledy připomínaly pravoúhlou drenážní síť, jinde celé úbočí vypadalo jako hrábě a horizont jako pilový kotouč. Na kontrolované trase přitom terén vystoupal přibližně z −644 m na +2 194 m mezi 20. a 55. kilometrem osy Z. V jednom místě byla půda téměř dva kilometry nad referenčním datem i bez místního kompresního zdvihu. V kódu původního experimentu se na výšce podílel široký interpolovaný příspěvek o amplitudě 4 200 m a další příspěvek až 5 200 m podél hranic oblastí oceánu. Lokální eroze navíc vedla koryta čtyřmi směry mřížky. To jsou konkrétní příklady toho, jak může technické rozhodnutí začít určovat vzhled celého světa.
Pro porovnání použij seed 42 ve stáří 4,6 miliardy let:
Poloha kamery původní hry (X, Y, Z) v metrech
Pozorovaná vada
(36.27, 2042.11, 43570.24)
Rozsáhlý vysoký vnitrozemský povrch. Směrem od pobřeží krajina převážně stoupá, lokální reliéf je málo čitelný.
(−288.53, 2474.74, 55305.27)
Řada téměř rovnoběžných zářezů s podobnou roztečí a zakončením, místy doprovázená drobnými vodními plochami.
(−418.08, 5459.46, 61443.66)
Dlouhá řada úzkých stejně orientovaných hřbetů a pravidelné zuby přes celý horizont.
Y zde znamená výšku očí, nikoli změřenou výšku terénu. U vlastní implementace zachovej pro kontrolu X/Z a přizpůsob výšku kamery novému povrchu. Po změně celého generátoru může na daném místě legitimně vzniknout moře. Proto navíc vyber nová místa se srovnatelným reliéfem a udělej kontrolu i na dalších seedech.
11. Podmínky pro předání výsledku
Spustitelný svět. Před vstupem lze zadat seed a stáří, svět prozkoumat pěšky i letem a dotázat se na stejné místo přes samostatné API simulace.
Vizuální ověření ve hře. Ukaž nezkrášlené snímky pobřeží nebo nížiny, lokálního pohoří se sedlem, říčního údolí, pískovcové oblasti s objemovým útvarem a horské vodní pánve, pokud ji model vytváří. Přilož seed, datum, souřadnice a měřítko. U alespoň jednoho místa ukaž pohled ze země i ze vzduchu.
Ověření měřítka a pravidelnosti. Změř rozdělení výšek suché země nad místní hladinou, převýšení na několika různě dlouhých trasách a orientaci výrazných zářezů. Na rovnoměrně nakloněné ploše se nesmí po každém řádku mřížky objevit hluboká rokle. Současně ověř, že skutečné soustředění odtoku údolí vytvoří. Průměrná výška celého světa tuto kontrolu nenahradí.
Ověření historie. Ukaž průběh stejného místa v několika stářích včetně změny materiálu nebo struktury. Ověř shodu při opakovaném dotazu, změně pořadí načítání a paralelním výpočtu. Velký skok do budoucnosti musí respektovat vyčerpání zdroje horniny.
Ověření zásahů a bilancí. Proveď test odtoku konečného oceánu do voidu, kaskády dvou propojených jezer s přelitím spodního a změny vodního toku hráčem s následným skokem o sto let. Sleduj vodu, sůl a přesunutý pevný materiál. Ukaž, kam každá měřená složka odešla.
Ověření objemové geometrie. Proleť pod přirozeně vzniklou skalní střechou nebo branou. V jednom sloupci ověř vzduch, pevný strop i povrch nad ním ve hráčské kolizi, ve voxelovém vzorku a v samotné simulaci.
Ověření výkonu. Měř dobu snímku i její horší percentily při rychlém letu do dosud nenačtené krajiny, zvlášť čas hlavní smyčky a čas generování. Ověř také zotavení po načtení terénu kolem hráče. Po testu zavři vlastní instance hry.
Poctivé vyhodnocení. Dodej přehled implementovaných procesů, použitých aproximací, profilovaných nákladů a zbývajících slabin. Jednu dobře vybranou horu doplň průletem běžnou krajinou. Právě tam bylo v původní implementaci nejlépe vidět, že se stejný mechanismus opakuje všude.
Rozhodující je, aby rozdíl mezi nížinou, starou vrchovinou, mladým hřebenem, říční soutěskou a skalním městem šel poznat už ze samotného tvaru. A aby po zásahu hráče tentýž svět dál fungoval podle pravidel, která ho vytvořila.
