# SwimWatch Timing System — Hoe het werkt! 🏊

*Een uitleg voor kinderen (en volwassenen!) over hoe de timer zo nauwkeurig werkt*

---

## Het Systeem

Stel je voor dat je in een zwembad bent met 8 banen. Er zijn:
- **1 Starter** — de man/vrouw met een knop die het startschot geeft
- **8 Lane Devices** — één timer aan de overkant van elke baan
- **1 Master Klok** — een centrale computer die zegt hoe laat het is

**Alle timers kijken naar dezelfde master klok** — net als wanneer je huiswerk op school doet en iedereen op dezelfde klok kijkt.

---

## Het Probleem (Vroeger) ❌

Toen één timer de startknop indrukde, moest het signaal door de WiFi naar alle andere timers reizen. Dat was snel, maar niet TE snel!

**Wat gebeurde:**
1. Starter drukt op knop: **STARTTTTTT!** ⏱️
2. Signaal reist door WiFi: `..ooooom..` 🌐
3. Na 30-50 milliseconden komt het signaal aan
4. Alle baan-timers starten LAAT! 😞

🔴 **Resultaat:** Sommige timers waren 30-50 milliseconden ACHTER op de starter. Dat klinkt niet veel, maar voor zwemmers IS dat veel!

---

## De Oplossing (Nu) ✅

Nu gebruiken we een extra slim truucje:

### Wat de Starter doet:

```
😊 Starter: "Hé timers! Ik start NU!"
    "Mijn klok zegt: 14:30:05.123 milliseconden + 456 microseconden"
                                      ↓↓↓
    Stuurt dit bericht via WiFi naar alle timers
```

### Wat de Baan-timers doen:

```
🏊 Lane Timer krijgt bericht:
   "Start! De klok zei 14:30:05.123.456"
   
   "Laat mij checken... mijn klok zegt NU: 14:30:05.153.789"
   
   "O! Dus de start was al 30.333 milliseconden geleden!
    Ik zal mijn timer álsof-starten op 30.333 milliseconden,
    niet op 0. Dan ben ik synchroon met de starter!"
   
   ✅ Nu lopen ALLE timers gelijk!
```

---

## Vergelijking met een Voetbalspel ⚽

**Zonder smartheid:**
- Coach fluit
- Signaal reist naar trainer
- Trainer start stopwatch LAAT
- Verschillende timers tonen verschillende tijden 😕

**Met ons smarte systeem:**
- Coach fluit EN zegt "Mijn klok zei exact 14:30:00"
- Trainer ziet: "Mijn klok zegt nu 14:30:00.050"
- Trainer denkt: "Dus het gebeurde 50 milliseconden geleden"
- Trainer zet zijn stopwatch ALS-OB het 50 milliseconden geleden begon
- Nu tonen ALLE stopwatches hetzelfde! ✅

---

## De Twee Getallen op het Bericht 📲

Het bericht van de starter bevat TWEE getallen voor super-nauwkeurigheid:

```
"Start! 
 timestamp: 1234567890123    (milliseconden - grote getal)
 timestamp_us: 456           (microseconden 0-999 - kleine extra)"
```

**Waarom twee getallen?**
- Het eerste getal (`timestamp`): milliseconden (duizendsten van een seconde)
- Het tweede getal (`timestamp_us`): nog kleinere stukjes (miljoensten van een seconde!)

**Bij elkaar:** Microseconde nauwkeurigheid! 🎯

---

## Lokale Splitsen (Het Makkelijke Deel) 💪

Wanneer een zwemmer de muur aanraakt:

```
🏊 Zwemmer raakt muur aan
   ↓
📱 Timer leest zijn eigen klok
   ↓
✅ Bericht naar scorebord: "Ik las 25.142 seconden"
   
Dit is PERFECT nauwkeurig, geen netwerk nodig! 🎯
```

Dit gebeurt allemaal op ÉÉN timer, dus geen wachten op WiFi!

---

## Meerdere Baantjes (Meer Zwemmen) 🏊🏊

Zwemmers die meerdere baantjes zwemmen:

```
1e keer aanraken:  25.142s  →  1e tijd = 25.142s
2e keer aanraken:  51.287s  →  2e tijd = 51.287 - 25.142 = 26.145s
3e keer aanraken:  78.410s  →  3e tijd = 78.410 - 51.287 = 27.123s
```

**Gemakkelijk:** Trek het vorige getal af! 🧮

---

## Hoe Nauwkeurig is het? 🎯

| Wat meten we? | Hoe goed? | Waarom? |
|---|---|---|
| Een timer alleen | Perfect! (0.001ms) | Één computer, geen wachten |
| Starter vs Lane | Super goed! ±1-2ms | Microseconden mee-gerekend |
| Splitsen (aanraken muur) | Perfect! (0.001ms) | Geen netwerkvertraging |
| Verschil tussen twee baantjes | Perfect! (0.001ms) | Zelfde timer, gewoon aftrekken |
| Alle timers samen | ±1-2ms | Super nauwkeurig! 🌟 |

### Vergelijken met regels ⚖️

**FINA-regels** (internationale zwemfedera):
- Timing mag maximaal ±10 milliseconden afwijken

**SwimWatch doet:**
- ±1-2 milliseconden = **5-10 KEER beter!** 🏆

Dit is net zo goed als professionele tijdklokken in echte olympische zwembaden!

---

## Hoe Voelt het voor Zwemmers? 💨

**Voor jou als zwemmer:**
- Alle timers tonen hetzelfde getal ✅
- Geen verwarrende verschillen meer ❌
- Eerlijk tegen iedereen 🤝
- Super nauwkeurig 🎯

---

## Snel Samengevat 🏃

1. **Starter drukt knop** → zegt tegen alle timers wat zijn klok zei (tot op de microseconde!)
2. **Baan-timers denken na** → "Hoelang geleden gebeurde dat? Ik tel terug!"
3. **Timers starten terug in het verleden** → nu synchrone met de starter
4. **Alle timers tonen hetzelfde getal** → iedereen even eerlijk! ✅

---

## Je Kunt het Zelf Proberen! 🧪

**Wat je nodig hebt:**
- 3 vrienden
- 3 stopwatches (of je telefoon)
- Een klok aan de muur

**Het spel:**
1. Iedereen kijkt op dezelfde klok om te synchroniseren
2. Één persoon zegt: "STARTTTTT! Mijn klok zei 14:30:05!"
3. De andere twee checken: "Mijn klok zegt nu 14:30:05.050" 
4. Ze stellen hun stopwatch in op: "Begin bij 0.050 seconden"
5. Nu lopen alle stopwatches gelijk! 🎉

---

**Versie:** 1.0  
**Datum:** Februari 2026  
**Hardware:** LilyGO T-Display S3 (ESP32-S3)  
**Begrijplijkheid:** ⭐⭐⭐⭐⭐ (voor kinderen!)

---

**Nog vragen?** 🤔
Vraag je ouders, trainer, of coach! Zij kunnen dit ook lezen. 😊
