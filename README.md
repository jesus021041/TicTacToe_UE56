
# Simulatore Strategico a Turni 3D

**Unreal Engine 5.6 | C++ & Blueprints**

## Inizio del Progetto

1. Avviare **Unreal Engine 5.6**
2. Configurare i parametri tramite GUI:

   * Dimensione della mappa (di base -> 25)
   * Seed (0 = generazione casuale)
   * Algoritmo IA:

     * A*
     * Greedy
3. Scegliere la modalità

---

###  Generazione Procedurale (Req. 1 - 2 – 3)

**Config**

* Gestita via UI iniziale
* Dati in `TTT_GameInstance.cpp`

**Descrizione**

* Implementata in `GameField.cpp`
* Basata su **Perlin Noise**
* 5 livelli di altezza (0 = acqua, invalicabile)
* Prevenzione automatica di nodi isolati
* Piazzamento Torri (N = 3)

---

###  Algortimo && implementazioni (Req. 4 - 8)

**Sistema a Turni & Pathfinding**

* Gestito da `TTT_GameMode.cpp`
* Algoritmo: **A***
* Costi movimento:

  * Salita: 2
  * Piano/discesa: 1
*  Movimento diagonale disabilitato

**Unità**

* Classe base: `BaseUnit`
* Classi derivate:

  * `Sniper`
  * `Brawler`

**Combattimento**

* Vincolo: impossibile attaccare nemici a quota superiore
* Sniper: contrattacco automatico a distanza

**Respawn**

* Ripristino unità alla posizione iniziale, una volta killate.

---

###  Interfaccia Utente (Req. 6 – 7 – 9)

**HUD**

* `UnitActionWidget`
* Visualizzazione stats e punteggi in tempo reale

**Feedback Visivo**

* Gestito da `TTT_PlayerController.cpp`
* Codifica colori:

  * 🟢 Movimento valido
  * 🔴 Nemico attaccabile

**Log Azioni**

* `ActionLogWidget.cpp`
* Esempi:

  ```
  AI: B S23 -> R19
  HP: S L2 -> L5
  ```

---

###  Obiettivi e Vittoria (Req. 5)

* 3 torri centrali conquistabili
* Stato neutrale in caso di conflitto
* Vittoria: controllo simultaneo di almeno **2 torri**

---

### Intelligenza Artificiale (Req. 10)

#### 🟠 Greedy Best-First (GreedyPlayer.cpp)
