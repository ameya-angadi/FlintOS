/*
 * Project Name: FlintOS
 * Designed For: TRMNL 7.5" (OG) DIY Kit By Seeed Studio
 *
 * License: GPL3+
 * This project is licensed under the GNU General Public License v3.0 or later.
 * You are free to use, modify, and distribute this software under the terms
 * of the GPL, as long as you preserve the original license and credit the original
 * author. For more details, see <https://www.gnu.org/licenses/gpl-3.0.en.html>.
 *
 * Copyright (C) 2026  Ameya Angadi
 *
 * Code Created And Maintained By: Ameya Angadi
 * Last Modified On: July 3, 2026
 * Version: 1.0.0
 *
 * Support my projects by purchasing through the following affiliate link:
 * TRMNL 7.5" (OG) DIY Kit by Seeed Studio - https://www.seeedstudio.com/TRMNL-7-5-Inch-OG-DIY-Kit-p-6481.html?sensecap_affiliate=JI84v1k&referring_service=link
 *
 */


#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "time.h"
#include "TFT_eSPI.h"
#include "load_screen_image.h"
#include "icons.h"
#include "slideshow_images.h"

// =========================================================================
// ========================== CONFIGURATION ZONE ===========================
// =========================================================================

// --- Wi-Fi Credentials ---
const char* WIFI_SSID = "";             // Paste your Wi-Fi SSID / Network Name here
const char* WIFI_PASSWORD = "";         // Paste your Wi-Fi Router Password here

// --- API Keys ---
const char* OWM_API_KEY = ""; // Paste your OpenWeatherMap API Key here
const char* CITY = "Bengaluru";                               // Paste your City Name
const char* COUNTRY_CODE = "IN";                              // Paste your Countey Code

const char* CMC_API_KEY = ""; // Paste your CoinMarketCap API Key here
const char* CMC_SYMBOLS = "BTC,ETH,SOL,DOGE";                 // Paste 4 codes of the Crypto Currency you want to track

const char* ZEN_API_KEY = ""; // Paste your ZenQuotes API Key here

const char* TODOIST_API_KEY = ""; // Paste your Todoist API Authorization Token here

// --- Cloud Script API Gateways ---
const char* STOCK_API_URL = ""; // Paste your deployed Google Apps Script Stock Web App URL link here

// --- NTP Time Server Configuration ---
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 19800;  // IST Timezone offset in seconds (e.g., 5.5 hours = 19800s)
const int DAYLIGHT_OFFSET_SEC = 0;

// --- Hardware Pins ---
const int BUTTON_PREV = D4;   
const int BUTTON_NEXT = D2;   
const int BUTTON_SELECT = D1; 

// --- Battery Diagnostic Engine Constants ---
#define BATTERY_PIN 1       // GPIO1 (A0) - BAT_ADC pin reader link
#define ADC_EN_PIN 6        // GPIO6 (A5) - ADC_EN power cutoff switch gate
const float CALIBRATION_FACTOR = 0.968;

// --- Timing Intervals (in milliseconds) ---
const unsigned long SCREEN_SWITCH_INTERVAL = 300000;    // Duration to switch between metric dashboard screens automatically (e.g., 5 mins)
const unsigned long DATA_FETCH_INTERVAL = 900000;       // Duration between system cloud synchronizations and API background fetches (e.g., 15 mins)
const unsigned long POPUP_INTERVAL = 1800000;           // Duration before the health and wellness self-care popup screen appears (e.g., 30 mins)
const unsigned long SLIDESHOW_IMAGE_INTERVAL = 300000;  // Duration of photo slideshow frame transitions when slideshow mode is active (e.g., 5 mins)

// =========================================================================

#ifdef EPAPER_ENABLE
EPaper epaper;
#endif

Preferences preferences;

// --- Master Mode State Machine ---
enum SystemMode {
  MODE_PRODUCTIVITY = 0,
  MODE_SLIDESHOW = 1
};
SystemMode currentMode = MODE_PRODUCTIVITY;

// --- Screen State Machine ---
enum ScreenState {
  SCREEN_HOME = 0,
  SCREEN_WEATHER_DETAILS = 1,
  SCREEN_CRYPTO = 2,
  SCREEN_STOCK = 3, 
  SCREEN_TODO = 4, 
  SCREEN_TIMER = 5
};
int currentScreen = SCREEN_HOME;
const int NUM_SCREENS = 6;

// --- Button Edge Detection States ---
bool lastStatePrev = HIGH;
bool lastStateNext = HIGH;
bool lastStateSel = HIGH;
unsigned long lastButtonPress = 0;
bool needsScreenUpdate = false; 
bool needsDataRefresh = false;  

// --- Global Tracking Variables ---
int lastMinute = -1;
unsigned long lastScreenSwitch = 0;
unsigned long lastDataFetch = 0;
unsigned long lastPopupTime = 0; 
unsigned long lastSlideshowSwitch = 0;
int currentSlideshowImage = 0;
bool isPopupActive = false;      

// --- Global Data Store: Weather & Quotes ---
String currentTemp = "--";
String feelsLikeTemp = "--"; 
String currentCondition = "--";
String weatherIconCode = "01d";
String currentHumidity = "--";
String currentWind = "--";
String currentPressure = "--";
String sunriseTime = "--:-- AM";
String sunsetTime = "--:-- PM";

String quoteText = "Loading daily inspiration...";
String quoteAuthor = "";

// --- Global Data Store: Crypto ---
String cryptoData[4] = { "BTC: --", "ETH: --", "SOL: --", "DOGE: --" };

// --- Global Data Store: Stocks ---
struct StockData {
  String symbol;
  String name;
  String price;
  String change;
};
StockData activeStocks[5];
int stockCount = 0;

// --- Global Data Store: Tasks ---
String activeTasks[5] = {"", "", "", "", ""};
int taskCount = 0;

// --- Global Data Store: Pomodoro Timer ---
bool isTimerRunning = false;
int currentSessionMinutes = 0;
int totalWorkMinutes = 0;
int lastSavedDay = -1;

// --- Layout Definitions ---
const int CENTER_X = 400;
const int CENTER_Y = 240;

// =========================================================================
// ==================== CORE USER DEFINED FUNCTIONS ========================
// =========================================================================

/* Evaluates tactile inputs, triggers chording windows, and processes navigation state switches. */
void handleButtons();

/* Orchestrates sequentially structured download streams from all paired web services. */
void fetchAllData();

/* Connects to OpenWeatherMap to download local weather states and sunrise/sunset records. */
void fetchWeatherData();

/* Parses cryptocurrency price arrays and twenty-four-hour asset trends from CoinMarketCap. */
void fetchCryptoData();

/* Retrieves the styled inspiration text frame and author signature from ZenQuotes. */
void fetchQuoteData();

/* Contacts the Google Cloud Web App script to compile personal market equity valuations. */
void fetchStockData();

/* Imports target schedules, content headers, and active daily task vectors from Todoist. */
void fetchTodoistData();

/* Drives structural screen clearing routines and redirects render flows to active templates. */
void switchScreen(int screenIndex);

/* Captures multiple high-speed analog samples to extract the precise real-world battery voltage. */
float readBatteryVoltage();

/* Translates raw analog lithium cell potentials into a standard 0 to 100 percent load reading. */
int calculateBatteryPercentage(float voltage);

/* Renders the productivity dashboard home template containing the layout timepiece and weather summary. */
void drawHomeScreen(struct tm& timeinfo);

/* Blits the size-scaled clock text strings centered at top boundaries. */
void drawHomeClock(struct tm& timeinfo);

/* Generates dual panel card modules mapping granular weather forecasts and environmental stats. */
void drawWeatherDetailsScreen();

/* Draws vertical cryptocurrency market rows tracking relative changes inside rounded plates. */
void drawCryptoScreen();

/* Builds horizontal corporate market tickers displaying symbols, company listings, and active valuation trends. */
void drawStockScreen();

/* Displays the focus stopwatch screen featuring session tracking controls. */
void drawTimerScreen();

/* Wipes local numerical bounding boxes to tick stopwatch clock numbers out of loop registers. */
void drawTimerNumbers();

/* Generates side-by-side components tracking active user tasks and monthly calendar grids. */
void drawTodoScreen();

/* Brings up an overlay layout reminding the user to rest, stretch, and hydrate. */
void drawHealthReminder();

/* Blits full-resolution 800x480 landscape background artwork files onto the display buffer. */
void drawSlideshowScreen(struct tm& timeinfo);

/* Draws floating high-contrast digital time slices in corners of active pictures. */
void drawSlideshowClock(struct tm& timeinfo);

/* Selects and balances vector alignments for incoming weather icon strings. */
void drawWeatherIcon(int x, int y, String iconCode);

/* Analyzes custom strings, wrapping lines tightly according to defined character widths. */
void drawWrappedStringCenter(String text, int yPos, int maxCharsPerLine);

void LoadBootScreen() {
#ifdef EPAPER_ENABLE
  epaper.drawBitmap(0, 0, LoadScreenImage, 800, 480, TFT_WHITE, TFT_BLACK);
  epaper.update();
  delay(2000);
#endif
}

void setup() {
  Serial.begin(115200);
  
  // Initialize Hardware Pins
  pinMode(BUTTON_PREV, INPUT_PULLUP);
  pinMode(BUTTON_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  
  pinMode(ADC_EN_PIN, OUTPUT);
  digitalWrite(ADC_EN_PIN, LOW); 
  analogReadResolution(12);
  analogSetPinAttenuation(BATTERY_PIN, ADC_11db);

  preferences.begin("trmnl_timer", false);
  totalWorkMinutes = preferences.getInt("workMin", 0);
  lastSavedDay = preferences.getInt("day", -1);

#ifdef EPAPER_ENABLE
  epaper.begin();
  LoadBootScreen();

  epaper.fillScreen(TFT_WHITE);
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(4);
  epaper.drawString("Connecting to WiFi...", CENTER_X, CENTER_Y);
  epaper.update();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

  // Wait explicitly for NTP sync to prevent 1970 dates
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.print("*");
  }

  epaper.fillScreen(TFT_WHITE);
  epaper.drawString("Fetching Live Data...", CENTER_X, CENTER_Y);
  epaper.update();

  fetchAllData();

  lastScreenSwitch = millis();
  lastPopupTime = millis(); 
  lastSlideshowSwitch = millis();
  switchScreen(SCREEN_HOME);
#endif
}

void loop() {
  handleButtons();

#ifdef EPAPER_ENABLE
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    delay(500);
    return;
  }

  // --- ASYNC EXECUTION BLOCKS ---
  if (needsDataRefresh) {
      epaper.fillScreen(TFT_WHITE);
      epaper.setTextColor(TFT_BLACK, TFT_WHITE);
      epaper.setTextDatum(MC_DATUM);
      epaper.setTextSize(3);
      epaper.drawString("Updating...", CENTER_X, CENTER_Y);
      epaper.update();

      if (currentScreen == SCREEN_HOME) {
        fetchWeatherData();
        fetchQuoteData();
      } else if (currentScreen == SCREEN_WEATHER_DETAILS) {
        fetchWeatherData();
      } else if (currentScreen == SCREEN_CRYPTO) {
        fetchCryptoData();
      } else if (currentScreen == SCREEN_STOCK) {
        fetchStockData();
      } else if (currentScreen == SCREEN_TODO) {
        fetchTodoistData();
      }
      
      lastDataFetch = millis(); 
      needsDataRefresh = false;
      needsScreenUpdate = true;
  }

  if (needsScreenUpdate) {
      if (currentMode == MODE_PRODUCTIVITY) {
          switchScreen(currentScreen);
      } else {
          epaper.fillScreen(TFT_WHITE);
          drawSlideshowScreen(timeinfo);
          epaper.update();
      }
      needsScreenUpdate = false;
  }

  // --- STANDARD BACKGROUND TASKS ---
  unsigned long currentMillis = millis();

  // --- 5-Minute Auto-Dismiss for Health Reminder ---
  // Automatically clear the popup if left unattended for 300,000ms (5 mins)
  if (isPopupActive && (currentMillis - lastPopupTime >= 300000)) {
      isPopupActive = false;
      lastPopupTime = currentMillis; // Restart the 30-min interval from this dismissal
      needsScreenUpdate = true;      // Force a redraw of the underlying screen
  }

  // Mode Specific Automated Switching Loops
  if (currentMode == MODE_SLIDESHOW) {
      if (!isPopupActive && (currentMillis - lastSlideshowSwitch >= SLIDESHOW_IMAGE_INTERVAL)) {
          lastSlideshowSwitch = currentMillis;
          currentSlideshowImage = (currentSlideshowImage + 1) % NUM_SLIDESHOW_IMAGES;
          needsScreenUpdate = true;
      }
  } else {
      if (!isPopupActive && (currentMillis - lastScreenSwitch >= SCREEN_SWITCH_INTERVAL)) {
          lastScreenSwitch = currentMillis;
          currentScreen = (currentScreen + 1) % NUM_SCREENS;
          needsScreenUpdate = true;
      }
  }

  if (!isPopupActive && (currentMillis - lastPopupTime >= POPUP_INTERVAL)) {
    isPopupActive = true;
    lastPopupTime = currentMillis;
    drawHealthReminder();
  }

  if (currentMillis - lastDataFetch >= DATA_FETCH_INTERVAL) {
    fetchAllData();
  }

  if (timeinfo.tm_min != lastMinute) {
    lastMinute = timeinfo.tm_min;

    if (timeinfo.tm_mday != lastSavedDay) {
      if (lastSavedDay != -1) totalWorkMinutes = 0;
      lastSavedDay = timeinfo.tm_mday;
      preferences.putInt("day", lastSavedDay);
      preferences.putInt("workMin", totalWorkMinutes);
    }

    if (isTimerRunning) currentSessionMinutes++;

    if (!isPopupActive && !needsScreenUpdate) {
      if (currentMode == MODE_SLIDESHOW) {
        int cx = 630, cy = 400, cw = 160, ch = 70;
        epaper.fillRect(cx, cy, cw, ch, TFT_BLACK);
        epaper.updataPartial(cx, cy, cw, ch);
        delay(20);
        epaper.fillRect(cx, cy, cw, ch, TFT_WHITE);
        epaper.updataPartial(cx, cy, cw, ch);
        delay(20);
        
        drawSlideshowClock(timeinfo);
        epaper.updataPartial(cx, cy, cw, ch);
      } 
      else if (currentScreen == SCREEN_HOME) {
        int clockWipeX = 100, clockWipeY = 30, wipeW = 600, wipeH = 140; 
        epaper.fillRect(clockWipeX, clockWipeY, wipeW, wipeH, TFT_BLACK);
        epaper.updataPartial(clockWipeX, clockWipeY, wipeW, wipeH);
        delay(20); 
        epaper.fillRect(clockWipeX, clockWipeY, wipeW, wipeH, TFT_WHITE);
        epaper.updataPartial(clockWipeX, clockWipeY, wipeW, wipeH);
        delay(20);

        drawHomeClock(timeinfo);
        epaper.updataPartial(clockWipeX, clockWipeY, wipeW, wipeH);
      } else if (currentScreen == SCREEN_TIMER) {
        int timerWipeX = 200, timerWipeY = 200, wipeW = 400, wipeH = 120;
        epaper.fillRect(timerWipeX, timerWipeY, wipeW, wipeH, TFT_BLACK);
        epaper.updataPartial(timerWipeX, timerWipeY, wipeW, wipeH);
        delay(20);
        epaper.fillRect(timerWipeX, timerWipeY, wipeW, wipeH, TFT_WHITE);
        epaper.updataPartial(timerWipeX, timerWipeY, wipeW, wipeH);
        delay(20);

        drawTimerNumbers();
        epaper.updataPartial(timerWipeX, timerWipeY, wipeW, wipeH);
      }
    }
  }

#endif

  delay(10); 
}

// =========================================================================
// ========================== HARDWARE CONTROLS ============================
// =========================================================================

void handleButtons() {
  bool statePrev = digitalRead(BUTTON_PREV);
  bool stateNext = digitalRead(BUTTON_NEXT);
  bool stateSel  = digitalRead(BUTTON_SELECT);

  // Debounce block
  if (millis() - lastButtonPress > 200) { 
    
    // Check if ANY button just got pressed (Edge Detection)
    if ((lastStatePrev == HIGH && statePrev == LOW) || 
        (lastStateNext == HIGH && stateNext == LOW) || 
        (lastStateSel == HIGH && stateSel == LOW)) {
        
        // Wait 80ms to give the user time to press the second button for a chord
        delay(80);
        
        // Re-read states to check if multiple buttons are currently held down
        statePrev = digitalRead(BUTTON_PREV);
        stateNext = digitalRead(BUTTON_NEXT);
        stateSel  = digitalRead(BUTTON_SELECT);

        // 1. DUAL BUTTON CHORD GESTURE DETECTION (Prev + Next Pressed Together)
        if (statePrev == LOW && stateNext == LOW) {
          currentMode = (currentMode == MODE_PRODUCTIVITY) ? MODE_SLIDESHOW : MODE_PRODUCTIVITY;
          lastSlideshowSwitch = millis();
          lastScreenSwitch = millis();
          needsScreenUpdate = true;
          lastButtonPress = millis() + 500; // Enforce longer cooldown to avoid accidental double-switching
          
          lastStatePrev = statePrev;
          lastStateNext = stateNext;
          lastStateSel = stateSel;
          return;
        }

        // 2. Health Reminder Dismissal Logic
        if (isPopupActive) {
          isPopupActive = false;
          lastPopupTime = millis();
          lastButtonPress = millis();
          needsScreenUpdate = true;
          
          lastStatePrev = statePrev;
          lastStateNext = stateNext;
          lastStateSel = stateSel;
          return; 
        }

        // 3. Standard Layout Navigation Routing
        if (currentMode == MODE_PRODUCTIVITY) {
          if (statePrev == LOW) { // Relying on the delayed read
            currentScreen = (currentScreen - 1 + NUM_SCREENS) % NUM_SCREENS;
            lastScreenSwitch = millis();
            needsScreenUpdate = true;
            lastButtonPress = millis();
          } else if (stateNext == LOW) {
            currentScreen = (currentScreen + 1) % NUM_SCREENS;
            lastScreenSwitch = millis();
            needsScreenUpdate = true;
            lastButtonPress = millis();
          } else if (stateSel == LOW) {
            if (currentScreen == SCREEN_TIMER) {
              isTimerRunning = !isTimerRunning;
              if (!isTimerRunning) {
                totalWorkMinutes += currentSessionMinutes;
                currentSessionMinutes = 0;
                preferences.putInt("workMin", totalWorkMinutes);
              }
              needsScreenUpdate = true;
            } else {
              needsDataRefresh = true; 
            }
            lastButtonPress = millis();
          }
        } 
        // 4. Slideshow Navigation Routing
        else if (currentMode == MODE_SLIDESHOW) {
          if (statePrev == LOW) {
            currentSlideshowImage = (currentSlideshowImage - 1 + NUM_SLIDESHOW_IMAGES) % NUM_SLIDESHOW_IMAGES;
            lastSlideshowSwitch = millis();
            needsScreenUpdate = true;
            lastButtonPress = millis();
          } else if (stateNext == LOW) {
            currentSlideshowImage = (currentSlideshowImage + 1) % NUM_SLIDESHOW_IMAGES;
            lastSlideshowSwitch = millis();
            needsScreenUpdate = true;
            lastButtonPress = millis();
          }
        }
    }
  }

  // Preserve edge state
  lastStatePrev = statePrev;
  lastStateNext = stateNext;
  lastStateSel = stateSel;
}

// =========================================================================
// ========================== BATTERY LOGIC ================================
// =========================================================================

float readBatteryVoltage() {
  digitalWrite(ADC_EN_PIN, HIGH);
  delay(10);
  
  long sum = 0;
  for(int i = 0; i < 30; i++) {
    sum += analogRead(BATTERY_PIN);
    delayMicroseconds(100);
  }
  digitalWrite(ADC_EN_PIN, LOW);
  
  float adc_avg = sum / 30.0;
  float voltage = (adc_avg / 4095.0) * 3.6 * 2.0 * CALIBRATION_FACTOR;
  return voltage;
}

int calculateBatteryPercentage(float voltage) {
  // Map standard 1S LiPo curve (4.2V Max, 3.2V Min)
  int percentage = (voltage - 3.2) * 100.0 / (4.2 - 3.2);
  if (percentage > 100) percentage = 100;
  if (percentage < 0) percentage = 0;
  return percentage;
}

// =========================================================================
// ======================== API DATA INGESTION =============================
// =========================================================================

void fetchAllData() {
  lastDataFetch = millis();
  
  WiFiClientSecure secureClient;
  secureClient.setInsecure();
  
  HTTPClient http;

  // 1. Weather
  http.begin(secureClient, "https://api.openweathermap.org/data/2.5/weather?q=" + String(CITY) + "," + String(COUNTRY_CODE) + "&appid=" + String(OWM_API_KEY) + "&units=metric");
  if (http.GET() == 200) {
    JsonDocument doc;
    deserializeJson(doc, http.getString());
    currentTemp = String((int)round((float)doc["main"]["temp"]));
    feelsLikeTemp = String((int)round((float)doc["main"]["feels_like"])); 
    currentCondition = String((const char*)doc["weather"][0]["description"]);
    currentCondition.toUpperCase();
    weatherIconCode = String((const char*)doc["weather"][0]["icon"]);
    currentHumidity = String((int)doc["main"]["humidity"]) + "%";
    currentPressure = String((int)doc["main"]["pressure"]) + " hPa";
    currentWind = String((float)doc["wind"]["speed"], 1) + " m/s";

    long sr = doc["sys"]["sunrise"]; long ss = doc["sys"]["sunset"]; struct tm *ti; time_t epoch; char buf[16];
    epoch = sr; ti = localtime(&epoch); strftime(buf, sizeof(buf), "%I:%M %p", ti); sunriseTime = String(buf);
    epoch = ss; ti = localtime(&epoch); strftime(buf, sizeof(buf), "%I:%M %p", ti); sunsetTime = String(buf);
  }
  http.end(); // Clean up socket connection for the next domain

  // 2. Crypto
  http.begin(secureClient, "https://pro-api.coinmarketcap.com/v1/cryptocurrency/quotes/latest?symbol=" + String(CMC_SYMBOLS));
  http.addHeader("X-CMC_PRO_API_KEY", CMC_API_KEY);
  http.addHeader("Accept", "application/json");
  if (http.GET() == 200) {
    JsonDocument doc;
    deserializeJson(doc, http.getString());
    const char* coins[] = { "BTC", "ETH", "SOL", "DOGE" };
    for (int i = 0; i < 4; i++) {
      if (doc["data"][coins[i]]) {
        float price = doc["data"][coins[i]]["quote"]["USD"]["price"];
        float change = doc["data"][coins[i]]["quote"]["USD"]["percent_change_24h"];
        String priceStr = (price < 1.0) ? String(price, 3) : String((int)round(price));
        cryptoData[i] = String(coins[i]) + ": $" + priceStr + " (" + String(change, 1) + "%)";
      }
    }
  }
  http.end(); // Clean up socket connection for the next domain

  // 3. Quotes
  http.begin(secureClient, "https://zenquotes.io/api/today?key=" + String(ZEN_API_KEY));
  if (http.GET() == 200) {
    JsonDocument doc;
    deserializeJson(doc, http.getString());
    quoteText = String((const char*)doc[0]["q"]);
    quoteAuthor = "- " + String((const char*)doc[0]["a"]);
  }
  http.end(); // Clean up socket connection for the next domain

  // 4. Todoist
  http.begin(secureClient, "https://api.todoist.com/api/v1/tasks?filter=due%20today");
  http.addHeader("Authorization", "Bearer " + String(TODOIST_API_KEY));
  if (http.GET() == 200) {
    DynamicJsonDocument doc(16384);
    if (!deserializeJson(doc, http.getString())) {
      taskCount = 0;
      JsonArray array = doc.is<JsonArray>() ? doc.as<JsonArray>() : doc["results"].as<JsonArray>();
      for (JsonVariant task : array) {
        if (taskCount >= 5) break;
        activeTasks[taskCount++] = String((const char*)task["content"]);
      }
    }
  }
  http.end(); // Clean up socket connection for the next domain

  // 5. Stocks (Requires strict redirect config changes)
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); 
  http.begin(secureClient, STOCK_API_URL);
  if (http.GET() == 200) {
    DynamicJsonDocument doc(4096);
    if (!deserializeJson(doc, http.getString())) {
      stockCount = 0;
      for (JsonVariant stock : doc["stocks"].as<JsonArray>()) {
        if (stockCount >= 5) break;
        activeStocks[stockCount].symbol = String((const char*)stock["symbol"]);
        activeStocks[stockCount].name = String((const char*)stock["name"]);
        activeStocks[stockCount].price = String((const char*)stock["price"]);
        activeStocks[stockCount].change = String((const char*)stock["change"]);
        stockCount++;
      }
    }
  }
  http.end(); // Clean up socket connection
}

// Fallback manual individual fetches for Select Button refresh
void fetchWeatherData() { fetchAllData(); }
void fetchCryptoData() { fetchAllData(); }
void fetchQuoteData() { fetchAllData(); }
void fetchStockData() { fetchAllData(); }
void fetchTodoistData() { fetchAllData(); }

// =========================================================================
// ======================== UI RENDERING LOGIC =============================
// =========================================================================

void switchScreen(int screenIndex) {
#ifdef EPAPER_ENABLE
  epaper.fillScreen(TFT_WHITE);
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  epaper.setTextDatum(MC_DATUM);

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  if (screenIndex == SCREEN_HOME) {
    drawHomeScreen(timeinfo);
  } else if (screenIndex == SCREEN_WEATHER_DETAILS) {
    drawWeatherDetailsScreen();
  } else if (screenIndex == SCREEN_CRYPTO) {
    drawCryptoScreen();
  } else if (screenIndex == SCREEN_STOCK) {
    drawStockScreen();
  } else if (screenIndex == SCREEN_TIMER) {
    drawTimerScreen();
  } else if (screenIndex == SCREEN_TODO) {
    drawTodoScreen();
  }

  epaper.update();  
#endif
}

// --- Photo Slideshow Mode Rendering ---
void drawSlideshowScreen(struct tm& timeinfo) {
#ifdef EPAPER_ENABLE
  // Blit current 800x480 photo asset out of flash memory container
  epaper.drawBitmap(0, 0, slideshow_images[currentSlideshowImage], 800, 480, TFT_WHITE, TFT_BLACK);
  drawSlideshowClock(timeinfo);
#endif
}

void drawSlideshowClock(struct tm& timeinfo) {
#ifdef EPAPER_ENABLE
  int cx = 630, cy = 400, cw = 160, ch = 70;
  
  epaper.fillRect(cx, cy, cw, ch, TFT_WHITE);
  epaper.drawRoundRect(cx, cy, cw, ch, 8, TFT_BLACK);
  
  char timeStr[12];
  strftime(timeStr, sizeof(timeStr), "%I:%M %p", &timeinfo);
  char dateStr[16];
  strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);
  
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  epaper.setTextDatum(MC_DATUM);
  
  epaper.setTextSize(3);
  epaper.drawString(timeStr, cx + cw / 2, cy + 25);
  
  epaper.drawLine(cx + 15, cy + 42, cx + cw - 15, cy + 42, TFT_BLACK);
  
  epaper.setTextSize(2);
  epaper.drawString(dateStr, cx + cw / 2, cy + 55);
#endif
}

// --- Health Reminder Popup Overlay ---
void drawHealthReminder() {
#ifdef EPAPER_ENABLE
  epaper.fillScreen(TFT_WHITE);
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);

  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(4);
  epaper.drawString("Wellness Check", CENTER_X, 50);
  epaper.drawLine(250, 80, 550, 80, TFT_BLACK);

  epaper.drawRoundRect(60, 110, 320, 130, 15, TFT_BLACK);
  epaper.setTextSize(3);
  epaper.drawString("Hydration", 220, 140);
  epaper.setTextSize(2);
  epaper.drawString("Take a moment to drink", 220, 180);
  epaper.drawString("water and rehydrate.", 220, 205);

  epaper.drawRoundRect(420, 110, 320, 130, 15, TFT_BLACK);
  epaper.setTextSize(3);
  epaper.drawString("Gratitude", 580, 140);
  epaper.setTextSize(2);
  epaper.drawString("Reflect on one thing", 580, 180);
  epaper.drawString("you are grateful for.", 580, 205);

  epaper.drawRoundRect(60, 260, 320, 130, 15, TFT_BLACK);
  epaper.setTextSize(3);
  epaper.drawString("Movement", 220, 290);
  epaper.setTextSize(2);
  epaper.drawString("Step away from the screen", 220, 330);
  epaper.drawString("and take a short walk.", 220, 355);

  epaper.drawRoundRect(420, 260, 320, 130, 15, TFT_BLACK);
  epaper.setTextSize(3);
  epaper.drawString("Mindset", 580, 290);
  epaper.setTextSize(2);
  epaper.drawString("Breathe deeply. You are", 580, 330);
  epaper.drawString("capable & making progress.", 580, 355);

  epaper.setTextSize(2);
  epaper.drawRoundRect(150, 425, 500, 30, 8, TFT_BLACK);
  epaper.drawString("Press any button to close the reminder", CENTER_X, 440);

  epaper.update();
#endif
}

// --- Home Screen Layout ---
void drawHomeScreen(struct tm& timeinfo) {
#ifdef EPAPER_ENABLE
  drawHomeClock(timeinfo);

  int tempCharWidth = 24; 
  int tempW = currentTemp.length() * tempCharWidth;
  int iconW = 64;
  int gap1 = 15; 
  int gap2 = 12; 
  int charCW = 24; 
  
  int totalGrpW = iconW + gap1 + tempW + gap2 + charCW;
  int startGrpX = CENTER_X - (totalGrpW / 2);

  int iconX = startGrpX + (iconW / 2); 
  int iconY = 210; 
  drawWeatherIcon(iconX, iconY, weatherIconCode);

  epaper.setTextDatum(BL_DATUM); 
  epaper.setTextSize(4);
  int textX = startGrpX + iconW + gap1;
  int alignBottomY = iconY + 28; 
  epaper.drawString(currentTemp.c_str(), textX, alignBottomY);

  int degreeX = textX + tempW + 6;
  epaper.drawCircle(degreeX, alignBottomY - 32, 3, TFT_BLACK);
  epaper.drawCircle(degreeX, alignBottomY - 32, 2, TFT_BLACK);
  
  epaper.drawString("C", degreeX + 8, alignBottomY);

  epaper.drawLine(200, 280, 600, 280, TFT_BLACK);
  
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(3);
  drawWrappedStringCenter(quoteText, 330, 36); // Wrap length tightened

  epaper.setTextSize(2);
  epaper.drawString(quoteAuthor, CENTER_X, 430);

  // Added Battery Percentage overlay bottom right
  float vBat = readBatteryVoltage();
  int batPct = calculateBatteryPercentage(vBat);
  
  String batStr = "[" + String(batPct) + "%]";
  epaper.setTextDatum(BR_DATUM);
  epaper.setTextSize(2);
  epaper.drawString(batStr, 770, 470);
  
  // Calculate Dynamic Left-Align math for the 32x16 Battery Icon
  int batStrLen = batStr.length() * 12; 
  int batteryX = 770 - batStrLen - 38; 
  
  // FIXED: Dynamically map the correct battery icon based on charge limit
  const unsigned char* currentBatIcon = (batPct > 50) ? icon_battery : icon_battery_half;
  epaper.drawBitmap(batteryX, 453, currentBatIcon, 32, 16, TFT_WHITE, TFT_BLACK);
#endif
}

void drawHomeClock(struct tm& timeinfo) {
#ifdef EPAPER_ENABLE
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  epaper.setTextDatum(MC_DATUM);

  char timeStr[12];
  strftime(timeStr, sizeof(timeStr), "%I:%M %p", &timeinfo); 
  char dateStr[32];
  strftime(dateStr, sizeof(dateStr), "%A, %d %b %Y", &timeinfo);

  epaper.setTextSize(3);
  epaper.drawString(dateStr, CENTER_X, 60);

  epaper.setTextSize(7);
  epaper.drawString(timeStr, CENTER_X, 130);
#endif
}

// --- Weather Screen Layout ---
void drawWeatherDetailsScreen() {
#ifdef EPAPER_ENABLE
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(4);
  epaper.drawString(CITY + String(" Weather"), CENTER_X, 40);

  int cardY = 90;
  int cardH = 320;
  int leftCardX = 50;
  int rightCardX = 420;
  int cardW = 330;

  epaper.fillRoundRect(leftCardX, cardY, cardW, cardH, 15, TFT_BLACK);
  epaper.fillRoundRect(leftCardX + 2, cardY + 2, cardW - 4, cardH - 4, 13, TFT_WHITE);

  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(2);
  epaper.drawString("LOCAL WEATHER", leftCardX + cardW / 2, cardY + 25);
  epaper.drawLine(leftCardX + 20, cardY + 45, leftCardX + cardW - 20, cardY + 45, TFT_BLACK);

  drawWeatherIcon(leftCardX + 165, cardY + 105, weatherIconCode);

  epaper.setTextSize(3);
  epaper.drawString(currentCondition, leftCardX + cardW / 2, cardY + 165);

  int tempCharWidth = 30; 
  int tempW = currentTemp.length() * tempCharWidth;
  int totalTempW = tempW + 12 + tempCharWidth; 
  int startTempX = leftCardX + (cardW / 2) - (totalTempW / 2);

  epaper.setTextDatum(ML_DATUM);
  epaper.setTextSize(5);
  epaper.drawString(currentTemp.c_str(), startTempX, cardY + 225);
  
  int degX = startTempX + tempW + 6;
  epaper.drawCircle(degX, cardY + 205, 4, TFT_BLACK);
  epaper.drawCircle(degX, cardY + 205, 3, TFT_BLACK);
  epaper.drawString("C", degX + 10, cardY + 225);
  
  int flCharW = 12; 
  String flText = "Feels Like: " + feelsLikeTemp;
  int flTextW = flText.length() * flCharW;
  int flTotalW = flTextW + 8 + flCharW;
  int flStartX = leftCardX + (cardW / 2) - (flTotalW / 2);

  epaper.setTextSize(2);
  epaper.drawString(flText.c_str(), flStartX, cardY + 280);
  
  int flDegX = flStartX + flTextW + 4;
  epaper.drawCircle(flDegX, cardY + 273, 2, TFT_BLACK);
  epaper.drawCircle(flDegX, cardY + 273, 1, TFT_BLACK);
  epaper.drawString("C", flDegX + 6, cardY + 280);

  epaper.fillRoundRect(rightCardX, cardY, cardW, cardH, 15, TFT_BLACK);
  epaper.fillRoundRect(rightCardX + 2, cardY + 2, cardW - 4, cardH - 4, 13, TFT_WHITE);

  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(2);
  epaper.drawString("FORECAST DATA", rightCardX + cardW / 2, cardY + 25);
  epaper.drawLine(rightCardX + 20, cardY + 45, rightCardX + cardW - 20, cardY + 45, TFT_BLACK);

  int listY = cardY + 95;

  epaper.setTextDatum(MR_DATUM);
  epaper.setTextSize(2); 
  epaper.drawString(currentHumidity, rightCardX + cardW - 25, listY);
  epaper.drawString(currentWind, rightCardX + cardW - 25, listY + 45);
  epaper.drawString(currentPressure, rightCardX + cardW - 25, listY + 90);
  epaper.drawString(sunriseTime, rightCardX + cardW - 25, listY + 135); 
  epaper.drawString(sunsetTime, rightCardX + cardW - 25, listY + 180);  

  epaper.setTextDatum(ML_DATUM);
  epaper.setTextSize(2); 
  epaper.drawString("Humidity:", rightCardX + 25, listY);
  epaper.drawString("Wind Speed:", rightCardX + 25, listY + 45);
  epaper.drawString("Pressure:", rightCardX + 25, listY + 90);
  epaper.drawString("Sunrise:", rightCardX + 25, listY + 135);
  epaper.drawString("Sunset:", rightCardX + 25, listY + 180);
#endif
}

// --- Crypto Screen Layout ---
void drawCryptoScreen() {
#ifdef EPAPER_ENABLE
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(4);
  epaper.drawString("Crypto Markets", CENTER_X, 40);

  if (String(CMC_API_KEY) == "YOUR_COINMARKETCAP_API_KEY") {
    epaper.setTextSize(3);
    epaper.drawString("API Key Required", CENTER_X, CENTER_Y);
    return;
  }

  int startY = 120;
  for (int i = 0; i < 4; i++) {
    int yCenter = startY + (i * 85);
    epaper.drawRoundRect(75, yCenter - 35, 650, 70, 15, TFT_BLACK);
    epaper.setTextDatum(MC_DATUM);
    epaper.setTextSize(4);
    epaper.drawString(cryptoData[i], CENTER_X, yCenter);
  }

  epaper.drawRoundRect(CENTER_X - 310, 445, 620, 25, 8, TFT_BLACK);
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(2);
  epaper.drawString("Data is updated from API and may not be the latest", CENTER_X, 458);

#endif
}

// --- Stock Market Screen Layout ---
void drawStockScreen() {
#ifdef EPAPER_ENABLE
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(4);
  epaper.drawString("Stock Market", CENTER_X, 40);

  if (String(STOCK_API_URL) == "YOUR_GOOGLE_SCRIPT_URL_HERE") {
    epaper.setTextSize(3);
    epaper.drawString("API Link Required", CENTER_X, CENTER_Y);
    return;
  }

  int startY = 85;
  int boxH = 60;
  int boxW = 760; 
  int startX = CENTER_X - (boxW / 2); 

  for (int i = 0; i < 5; i++) {
    int yPos = startY + (i * (boxH + 12));
    epaper.drawRoundRect(startX, yPos, boxW, boxH, 10, TFT_BLACK);
    
    if (i < stockCount) {
      epaper.setTextDatum(ML_DATUM);
      epaper.setTextSize(2); 
      epaper.drawString(activeStocks[i].symbol, startX + 20, yPos + 30);

      String shortName = activeStocks[i].name;
      if (shortName.length() > 20) {
        shortName = shortName.substring(0, 18) + "..";
      }
      epaper.drawString(shortName, startX + 220, yPos + 30);

      epaper.setTextDatum(MR_DATUM);
      epaper.drawString(activeStocks[i].price, startX + boxW - 140, yPos + 30);
      
      // Calculate dynamic 16x16 vector math relative to price string length
      int priceStrLen = activeStocks[i].price.length() * 12; 
      int rupeeX = startX + boxW - 140 - priceStrLen - 20;
      epaper.drawBitmap(rupeeX, yPos + 22, icon_rupee, 16, 16, TFT_WHITE, TFT_BLACK);

      epaper.drawString(activeStocks[i].change, startX + boxW - 20, yPos + 30);
    } else {
      epaper.setTextDatum(MC_DATUM);
      epaper.setTextSize(2);
      epaper.drawString("Loading...", CENTER_X, yPos + 30);
    }
  }

  epaper.drawRoundRect(CENTER_X - 310, 445, 620, 25, 8, TFT_BLACK);
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(2);
  epaper.drawString("Data is updated from API and may not be the latest", CENTER_X, 458);

#endif
}

// --- Task & Calendar Screen Layout ---
void drawTodoScreen() {
#ifdef EPAPER_ENABLE
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  int boxW = 370;
  int boxH = 440;
  int leftX = 20;
  int rightX = 410;
  int topY = 20;

  epaper.drawRoundRect(leftX, topY, boxW, boxH, 15, TFT_BLACK);
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(4);
  epaper.drawString("Today's Tasks", leftX + boxW / 2, topY + 35);
  epaper.drawLine(leftX + 20, topY + 65, leftX + boxW - 20, topY + 65, TFT_BLACK);

  if (taskCount == 0) {
    epaper.setTextSize(3);
    epaper.drawString("No Tasks!", leftX + boxW / 2, topY + 220);
  } else {
    epaper.setTextDatum(ML_DATUM);
    int itemY = topY + 110;
    for (int i = 0; i < taskCount; i++) {
      epaper.setTextSize(2); 
      String taskStr = activeTasks[i];
      int maxChars = 24; 
      int currentY = itemY;
      if (taskStr.length() > maxChars) currentY -= 12; 

      String currentLine = "";
      while (taskStr.length() > 0) {
        int spaceIdx = taskStr.indexOf(' ');
        String word = (spaceIdx == -1) ? taskStr : taskStr.substring(0, spaceIdx);
        if (currentLine.length() + word.length() > maxChars) {
          epaper.drawString(currentLine, leftX + 60, currentY);
          currentLine = word + " ";
          currentY += 24; 
        } else {
          currentLine += word + " ";
        }
        if (spaceIdx == -1) break;
        taskStr = taskStr.substring(spaceIdx + 1);
      }
      if (currentLine.length() > 0) epaper.drawString(currentLine, leftX + 60, currentY);
      
      epaper.drawCircle(leftX + 35, itemY, 8, TFT_BLACK);
      epaper.drawCircle(leftX + 35, itemY, 7, TFT_BLACK);
      itemY += 65;
    }
  }

  epaper.drawRoundRect(rightX, topY, boxW, boxH, 15, TFT_BLACK);
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(4);
  
  char monthYearStr[32];
  strftime(monthYearStr, sizeof(monthYearStr), "%B %Y", &timeinfo);
  epaper.drawString(monthYearStr, rightX + boxW / 2, topY + 35);
  epaper.drawLine(rightX + 20, topY + 65, rightX + boxW - 20, topY + 65, TFT_BLACK);

  int currentDay = timeinfo.tm_mday;
  int currentMonth = timeinfo.tm_mon; 
  int currentYear = timeinfo.tm_year + 1900;
  int currentWday = timeinfo.tm_wday; 

  int daysInMonths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (currentMonth == 1 && ((currentYear % 4 == 0 && currentYear % 100 != 0) || (currentYear % 400 == 0))) {
    daysInMonths[1] = 29; 
  }
  
  int totalDays = daysInMonths[currentMonth];
  int firstDayOfWeek = (currentWday - (currentDay - 1) % 7 + 7) % 7;
  int startGridX = rightX + 18; 
  int startGridY = topY + 110;
  int cellW = 48;
  int cellH = 48;

  const char* dayNames[] = {"S", "M", "T", "W", "T", "F", "S"};
  epaper.setTextSize(3);
  for (int i = 0; i < 7; i++) {
    epaper.drawString(dayNames[i], startGridX + (i * cellW) + (cellW / 2), startGridY);
  }

  startGridY += 45;
  int dayCounter = 1;
  for (int row = 0; row < 6; row++) {
    for (int col = 0; col < 7; col++) {
      if ((row == 0 && col < firstDayOfWeek) || dayCounter > totalDays) continue; 
      int cx = startGridX + (col * cellW) + (cellW / 2);
      int cy = startGridY + (row * cellH);

      if (dayCounter == currentDay) {
        epaper.fillRoundRect(cx - 20, cy - 20, 40, 40, 6, TFT_BLACK);
        epaper.setTextColor(TFT_WHITE, TFT_BLACK);
        epaper.drawString(String(dayCounter), cx, cy);
        epaper.setTextColor(TFT_BLACK, TFT_WHITE); 
      } else {
        epaper.drawString(String(dayCounter), cx, cy);
      }
      dayCounter++;
    }
  }
#endif
}

// --- Timer Screen Layout ---
void drawTimerScreen() {
#ifdef EPAPER_ENABLE
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(4);
  epaper.drawString("Focus Stopwatch", CENTER_X, 80);

  epaper.setTextSize(3);
  if (isTimerRunning) {
    epaper.setTextColor(TFT_WHITE, TFT_BLACK);
    epaper.fillRoundRect(CENTER_X - 100, 130, 200, 50, 15, TFT_BLACK); 
    epaper.drawString("RUNNING", CENTER_X, 155);
    epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  } else {
    epaper.drawRoundRect(CENTER_X - 100, 130, 200, 50, 15, TFT_BLACK); 
    epaper.drawRoundRect(CENTER_X - 99, 131, 198, 48, 14, TFT_BLACK); 
    epaper.drawString("STOPPED", CENTER_X, 155);
  }

  epaper.drawLine(250, 195, 550, 195, TFT_BLACK);
  epaper.drawLine(250, 325, 550, 325, TFT_BLACK);

  drawTimerNumbers();

  epaper.setTextSize(3);
  epaper.drawString("Total Today: " + String(totalWorkMinutes) + " mins", CENTER_X, 370);

  epaper.drawRoundRect(CENTER_X - 160, 420, 320, 30, 8, TFT_BLACK);
  epaper.setTextSize(2);
  epaper.drawString("[ Select ] to Start / Stop", CENTER_X, 435);
#endif
}

void drawTimerNumbers() {
#ifdef EPAPER_ENABLE
  epaper.setTextColor(TFT_BLACK, TFT_WHITE);
  epaper.setTextDatum(MC_DATUM);
  epaper.setTextSize(8);
  epaper.drawString(String(currentSessionMinutes) + " m", CENTER_X, 260);
#endif
}

// --- UI Helper Functions ---
void drawWrappedStringCenter(String text, int startY, int maxCharsPerLine) {
#ifdef EPAPER_ENABLE
  int currentY = startY;
  String currentLine = "";

  int spaceIdx = 0;
  while (text.length() > 0) {
    spaceIdx = text.indexOf(' ');
    String word = (spaceIdx == -1) ? text : text.substring(0, spaceIdx);

    if (currentLine.length() + word.length() > maxCharsPerLine) {
      currentLine.trim(); 
      epaper.drawString(currentLine, CENTER_X, currentY);
      currentLine = word + " ";
      currentY += 40;
    } else {
      currentLine += word + " ";
    }

    if (spaceIdx == -1) break;
    text = text.substring(spaceIdx + 1);
  }
  if (currentLine.length() > 0) {
    currentLine.trim(); 
    epaper.drawString(currentLine, CENTER_X, currentY);
  }
#endif
}

void drawWeatherIcon(int x, int y, String iconCode) {
#ifdef EPAPER_ENABLE
  const unsigned char* iconArray = icon_default; 
  if (iconCode == "01d" || iconCode == "01n") iconArray = icon_01;
  else if (iconCode == "02d" || iconCode == "02n") iconArray = icon_02;
  else if (iconCode == "03d" || iconCode == "03n" || iconCode == "04d" || iconCode == "04n") iconArray = icon_03;
  else if (iconCode == "09d" || iconCode == "09n" || iconCode == "10d" || iconCode == "10n") iconArray = icon_09;
  else if (iconCode == "11d" || iconCode == "11n") iconArray = icon_11;
  else if (iconCode == "13d" || iconCode == "13n") iconArray = icon_13;
  else if (iconCode == "50d" || iconCode == "50n") iconArray = icon_50;
  epaper.drawBitmap(x - 32, y - 24, iconArray, 64, 64, TFT_WHITE, TFT_BLACK);
#endif
}