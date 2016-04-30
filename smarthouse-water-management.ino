#include <DS3231.h>
#include <PCF8574.h>      // Для PCF8574
#include <U8glib.h>       // Для OLED-дисплея
#include <EEPROM.h>       // Для работы с EEPROM
#include <Wire.h>         // Для I2C

/* DS3231 */
DS3231 clock;

/* Переменные работы кнопок */
#define NextPageButtonPin 2               // Вход к которому подключена кнопка "Далее"

// Временно убрал PIN с кнопки, т.к. планируется сделать блок кнопок с резисторами и подключением к одному порту (PIN 2) - PIN3 - теперь используется в регулировке подсветки LCD12864
//#define PrevPageButtonPin 3             // Вход к которому подключена кнопка "Назад"

int LongTapNext = 0;                      // Флаг для длительного нажатия кнопки - чтобы при удержании "Далее"/"Назад" не было цикличного перехода по страницам
//int LongTapPrev = 0;                      // Флаг для длительного нажатия кнопки - чтобы при удержании "Далее"/"Назад" не было цикличного перехода по страницам

// Инициализация LCD12864 (SPI)
U8GLIB_ST7920_128X64_4X u8g(13, 5, 4);
#define BLK 3                             // PIN(3) в Arduino Nano с PWM - используется для регулировки подсветки LCD12864
int WorkBrightness = 170;                 // Значение яркости LCD12864 в рабочем режиме
int SleepBrightness = 10;                 // Значение яркости LCD12864 в спящем режиме
bool SleepMode = false;                   // Текщий режим - "Режим сна"
const int BrightnessInterval = 30000;     // 30 секунд интервал для перехода в режим сна

// Инициализация расширителя портов PCF8574
PCF8574 pcf;

/* ----------------------- Иконки и значки ------------------------------ */
const uint8_t ico_err_status[] U8G_PROGMEM =
{
  // @0 'V' (11 pixels wide)
  0x1F, 0x00, //    #####
  0x20, 0x80, //   #     #
  0x40, 0x40, //  #       #
  0x91, 0x20, // #  #   #  #
  0x8A, 0x20, // #   # #   #
  0x84, 0x20, // #    #    #
  0x8A, 0x20, // #   # #   #
  0x91, 0x20, // #  #   #  #
  0x40, 0x40, //  #       #
  0x20, 0x80, //   #     #
  0x1F, 0x00, //    #####
};

const uint8_t ico_ok_status[] U8G_PROGMEM =
{
  // @0 'a' (8 pixels wide)
  0x01, //        #
  0x03, //       ##
  0x87, // #    ###
  0xCF, // ##  ####
  0xFF, // ########
  0xFE, // #######
  0xFC, // ######
  0x78, //  ####
  0x30, //   ##
};

const uint8_t ico_hotwater_blocked[] U8G_PROGMEM =
{
  0x44, 0x44, 0x40, //  #   #   #   #   #
  0x22, 0x22, 0x20, //   #   #   #   #   #
  0x11, 0x11, 0x10, //    #   #   #   #   #
  0x51, 0x11, 0x18, //  # #   #   #   #   ##
  0xD1, 0x11, 0x1C, // ## #   #   #   #   ###
  0xE2, 0x22, 0x2C, // ###   #   #   #   # ##
  0xE2, 0x22, 0x2C, // ###   #   #   #   # ##
  0xC4, 0x44, 0x4C, // ##   #   #   #   #  ##
  0xC4, 0x44, 0x4C, // ##   #   #   #   #  ##
  0xEA, 0xAA, 0xAC, // ### # # # # # # # # ##
  0xD5, 0x55, 0x5C, // ## # # # # # # # # ###
  0xEA, 0xAA, 0xAC, // ### # # # # # # # # ##
  0xD5, 0x55, 0x5C, // ## # # # # # # # # ###
  0xEA, 0xAA, 0xAC, // ### # # # # # # # # ##
  0xD5, 0x55, 0x5C, // ## # # # # # # # # ###
  0xFF, 0xFF, 0xFC, // ######################
};

const uint8_t ico_water_temp[] U8G_PROGMEM =
{
  0x03, 0xE6, //       #####  ##
  0x02, 0x49, //       #  #  #  #
  0x02, 0xE9, //       # ### #  #
  0x02, 0x46, //       #  #   ##
  0x02, 0xE0, //       # ###
  0x02, 0x40, //       #  #
  0x7A, 0xE0, //  #### # ###
  0x86, 0x40, // #    ##  #
  0x02, 0x40, //       #  #
  0x04, 0x21, //      #    #    #
  0x08, 0x1E, //     #      ####
  0x08, 0x10, //     #      #
  0x7F, 0xF0, //  ###########
  0x8F, 0xF9, // #   #########  #
  0x07, 0xE6, //      ######  ##
  0x03, 0xC0, //       ####
};

const uint8_t ico_air_temp[] U8G_PROGMEM =
{
  0x18, 0x06, //    ##        ##
  0x24, 0x09, //   #  #      #  #
  0x34, 0x09, //   ## #      #  #
  0x24, 0xE6, //   #  #  ###  ##
  0x35, 0x10, //   ## # #   #
  0x25, 0x00, //   #  # #
  0x35, 0x00, //   ## # #
  0x25, 0x10, //   #  # #   #
  0x34, 0xE0, //   ## #  ###
  0x66, 0x00, //  ##  ##
  0xC3, 0x00, // ##    ##
  0x81, 0x00, // #      #
  0xFF, 0x00, // ########
  0xFF, 0x00, // ########
  0x7E, 0x00, //  ######
  0x3C, 0x00, //   ####
};

const uint8_t left_arrow[] U8G_PROGMEM = {
  // @0 '3' (5 pixels wide)
  0x08, //     #
  0x18, //    ##
  0x38, //   ###
  0x78, //  ####
  0xF8, // #####
  0xF8, // #####
  0x78, //  ####
  0x38, //   ###
  0x18, //    ##
  0x08, //     #
};

const uint8_t right_arrow[] U8G_PROGMEM = {
  // @10 '4' (5 pixels wide)
  0x80, // #
  0xC0, // ##
  0xE0, // ###
  0xF0, // ####
  0xF8, // #####
  0xF8, // #####
  0xF0, // ####
  0xE0, // ###
  0xC0, // ##
  0x80, // #
};

const uint8_t ico_eye[] U8G_PROGMEM = {
  // @0 'N' (15 pixels wide)
  0x07, 0xC0, //      #####
  0x1B, 0xF0, //    ## ######
  0x20, 0x48, //   #      #  #
  0x41, 0xA4, //  #     ## #  #
  0x82, 0xA2, // #     # # #   #
  0x8B, 0xA2, // #   # ### #   #
  0x49, 0x24, //  #  #  #  #  #
  0x24, 0x48, //   #  #   #  #
  0x17, 0xD0, //    # ##### #
  0x0F, 0xE0, //     #######
};
/* END ------------------- Иконки и значки -------------------------- END */

int temp_cm1, cm1 = 0;                     // Расстояние от ультразвукового датчика до поверхности воды в домашней емкости (в см.)
int temp_cm2, cm2 = 0;                     // Расстояние от ультразвукового датчика до поверхности воды в уличной емкости (в см.)

const int averageFactor = 4;               // коэффициент сглаживания показаний (0 = не сглаживать) - чем выше, тем больше "инерционность" - это видно на индикаторах

//-------------------------------------------------------- Параметры емкостей ----------------------------------------------------------------------
//--------- Домашняя емкость --------
int MainTank_Height       = 94;            // Высота домашней емкости в см.
int MainTank_Max          = 5;              // Максимальный уровень воды домашней емкости в см. (от датчика до поверхности воды)
int MainTank_MinHotWater  = 60;             // Минимальное количество воды в домашней емкости в % чтобы заблокировать пополнение бака горячей воды
int MainTank_FillIn       = 80;             // Уровень воды в домашней емкости в % при котором включается режим наполнения
int MainTank_MinBlock     = 10;             // Минимальное количество воды в домашней емкости в % чтобы заблокировать работу насосной станции
int HotWater_Unblock      = 70;             // Количество воды в домашней емкости в % при котором разблокируется пополнение бака горячей воды

//-------- Уличная емкость ----------
int StreetTank_Height         = 94;        // Высота домашней емкости в см.
int StreetTank_Max            = 5;          // Максимальный уровень воды домашней емкости в см. (от датчика до поверхности воды)
int StreetTank_MinBlock       = 10;         // Минимальное количество воды уличной емкости в % чтобы заблокировать работу насоса
int StreetTank_WaterQuality   = 93;         // Качество воды в уличных емкостях (прозрачность)

//-------- Статусы емкостей ---------
int MainTankStatus = 1;                     // Статус домашней емкости
int StreetTankStatus = 1;                   // Статус уличной емкости
int HotWaterTankStatus = 1;                 // Статус бака с горячей водой

//------ Уровни емкостей в % и объем в литрах ------
int MainTankPercent       = 0;              // Объем воды в % для домашней емкости
int StreetTankPercent     = 0;              // Объем воды в % для уличной емкости
int HotWaterTankPercent   = 0;              // Объем воды в % для бака с горячей водой

//------ Переменные для работы с таймерами ---------
int progress                      = 0;      // Переменная для цикличного индикатора процесса работы (анимирования символов)
unsigned long previousMillisBLK   = 0;      // Счетчик для регулировки подсветки LCD12864 (режим сна через BrightnessInterval)
unsigned long currentMillis       = 0;

// Реализация таймера с заданным интервалом - пока закомментировано
//const long interval = 5000;                 // Интервал таймера
//unsigned long previousMillis      = 0;      // will store last time LED was updated

//----------- Переменные для графиков --------------
//int x1 = 0;                                 // Координата графика page3() - график температуры воздуха
//int y1 = 0;

// Блокировки емкостей и насосов
bool HotWaterTankIsBlocked = false;         // Заблокировано ли пополнение бака горячей воды
bool MainPumpIsBlocked = false;             // Насосная станция заблокирована

//-------------------- Переменные для работы со страницами -------------------------
int CurrentPage = 1;                       // Текущая страница: 1 - данные главной емкости, 2 - данные бака горячей воды, 3 - данные уличной емкости, 4 - скважина, 5 - Настройки

//-------------------- Номера реле в расширителе портов 8574 -----------------------
const int MainTankRelay      = 0;          // Насосная станция (0-й порт)
const int StreetTankRelay    = 1;          // Насос уличных емкостей (1-й порт)
const int HotWaterTankRelay  = 2;          // Электроклапан емкости с горячей водой (2-порт)
const int ChinkRelay         = 3;          // Скважинный насос

// Резерв портов на блоке реле - в последующем дать переменным нормальные имена
const int port5 = 4;
const int port6 = 5;
const int port7 = 6;
const int port8 = 7;

// ----------------------------- Температуры --------------------------------
float SlaveHotWater_temp  = 0;    // Температура полученная со Slave-контроллера
float HotWater_Temp       = 0;    // Температура горячей воды - используется для усреднения значений
int dataA                 = 0;    // Переменная для целой части температуры
int dataB                 = 0;    // Переменная для дробной части температуры

// Температура воздуха с модуля RTC
int   HomeAir_Temp              = 0;

void setup()
{
  /* Инициализация i2c */
  Wire.begin();

  /* Перевернем экран на 180 градусов */
  u8g.setRot180();

  /* Устанавливаем PIN(3) на выход для регулировки яркости подсветки LCD12864 */
  pinMode(BLK, OUTPUT);
  LED12864_Brightness(WorkBrightness);

  /* Инициализация I2C для расширителя портов PCF8574
    Адрес согласно установленных перемычек A0-A1-A2 (0x27 - без перемычек) */
  pcf.begin(0x27);

  /* Установка всех 8 портов PCF8574 на выход */
  pcf.pinMode(0, OUTPUT);
  pcf.pinMode(1, OUTPUT);
  pcf.pinMode(2, OUTPUT);
  pcf.pinMode(3, OUTPUT);
  pcf.pinMode(4, OUTPUT);
  pcf.pinMode(5, OUTPUT);
  pcf.pinMode(6, OUTPUT);
  pcf.pinMode(7, OUTPUT);
  pcf.set();

  // Проводим сброс состояния портов на расширителе PCF8574
  PCFReset();

  HotWaterTankIsBlocked   = EEPROM.read(0);
  MainTankStatus          = EEPROM.read(1);
  StreetTankStatus        = EEPROM.read(2);
  HotWaterTankStatus      = EEPROM.read(3);
  MainPumpIsBlocked       = EEPROM.read(5);

  if (MainTankStatus == 255)
  {
    MainTankStatus = 1;
    EEPROM.write(1, MainTankStatus);
  }

  if (StreetTankStatus == 255)
  {
    StreetTankStatus = 1;
    EEPROM.write(2, StreetTankStatus);
  }

  if (HotWaterTankStatus == 255)
  {
    HotWaterTankStatus = 1;
    EEPROM.write(3, HotWaterTankStatus);
  }

  if (MainPumpIsBlocked == 255)
  {
    MainPumpIsBlocked = 0;
    EEPROM.write(5, MainPumpIsBlocked);
  }

  if (MainPumpIsBlocked == true)
  {
    TurnOn(MainTankRelay);
  }

  if (StreetTankStatus = 3)
  {
    TurnOn(StreetTankRelay);
  }

  CurrentPage = 1;                // Инициализируем всегда 1-ю страницу при включении

  Serial.begin(9600); // Starting Serial Terminal
  //  Serial.println("----- Initializing system -----");
  //  Serial.print("Hot water tank block-status: ");
  //  Serial.print(HotWaterTankIsBlocked);
  //  Serial.println();
  //  Serial.print("MainTank status: ");
  //  Serial.print(TankStatus(MainTankStatus));
  //  Serial.println();
  //  Serial.print("StretTank status: ");
  //  Serial.print(TankStatus(StreetTankStatus));
  //  Serial.println();
  //  Serial.print("StretPump status: ");
  //  Serial.print(StreetPumpIsBlocked);
  //  Serial.println();
  //
  //  Serial.print("HotWaterTank status: ");
  //  Serial.print(TankStatus(HotWaterTankStatus));
  //  Serial.println();
  //  Serial.print("MainPump is blocked: ");
  //    Serial.print(MainPumpIsBlocked);
  //  Serial.println(cm1);

  // Инициализируем PIN для кнопок
  pinMode(NextPageButtonPin, INPUT_PULLUP);

  // Закомментирована кнопка PREV - планируется перенос кнопок в блок в резисторами (работа всех кнопок через PIN(2)
  //pinMode(PrevPageButtonPin, INPUT_PULLUP);

}

void loop()
{

  if (Serial.available() > 0) {  //если есть доступные данные
    // считываем байт
    //StreetTank_WaterQuality = Serial.parseInt();
    HotWaterTankPercent = Serial.parseInt();
    // отсылаем то, что получили
    Serial.print("I received: ");
    //Serial.println(StreetTank_WaterQuality, DEC);
    Serial.println(HotWaterTankPercent, DEC);
  }

  // Запрос к Slave-контроллеру для получения показаний датчиков
  i2cReadSlave();

  //Изменяем счетчик для анимации иконок, курсоров на OLED
  SystemCounters();

  // Обработка кнопок
  ProcessButtons();

  // Отображение уровня воды в емкостях
  MainTankLevel();
  StreetTankLevel();
  HotWaterTankLevel();

  // Отображение температуры с термопары на MAX6675 (температура в баке с горячей водой) и с DS3231 (температура в доме)
  HotWaterTemp();
  HomeAirTemp();

  // Вывод страниц на OLED
  ProcessPages();

  Logic_StreetTank();
  Logic_MainTank();
  Logic_HotWaterTank();

  WriteStates();
}   /* END LOOP */

// Получение уровня домашней емкости в %
void MainTankLevel()
{
  int oldsensorValue = cm1;
  cm1 = temp_cm1;
  //cm1 = ultrasonic1.Ranging(CM);

  if ((cm1 > MainTank_Height) || (cm1 < 0))
  {
    cm1 = MainTank_Height + MainTank_Max + averageFactor;
  }
  cm1 = (oldsensorValue * (averageFactor - 1) + cm1) / averageFactor;
  MainTankPercent = 100 - ((cm1 - MainTank_Max) * 100 / MainTank_Height);             //Текущий уровень в %
}

// Получение уровня уличной емкости в %
void StreetTankLevel()
{
  int oldsensorValue = cm2;
  cm2 = temp_cm2;
  //cm2 = ultrasonic2.Ranging(CM);

  if ((cm2 > StreetTank_Height) || (cm2 < 0))
  {
    cm2 = StreetTank_Height + StreetTank_Max + averageFactor;
  }
  cm2 = (oldsensorValue * (averageFactor - 1) + cm2) / averageFactor;
  StreetTankPercent = 100 - ((cm2 - StreetTank_Max) * 100 / StreetTank_Height);             //Текущий уровень в %
}

// Получение уровня бака с горячей водой в %
// 4 фиксированных положения "пусто = 0", "минимум = 1", "середина = 2", "максимум = 3"
void HotWaterTankLevel()
{
  //HotWaterTankPercent = 0;  //пусто
  //HotWaterTankPercent = 1;  //минимум
  //HotWaterTankPercent = 2;  //середина
  //HotWaterTankPercent = 3;  //максимум
}

// Обнуление (выключение) всех портов PCF8574
void PCFReset()
{
  pcf.digitalWrite(0, HIGH);
  pcf.digitalWrite(1, HIGH);
  pcf.digitalWrite(2, HIGH);
  pcf.digitalWrite(3, HIGH);
  pcf.digitalWrite(4, HIGH);
  pcf.digitalWrite(5, HIGH);
  pcf.digitalWrite(6, HIGH);
  pcf.digitalWrite(7, HIGH);
}

// Включение порта на PCF8574
void TurnOn(int val)
{
  pcf.digitalWrite(val, LOW);
}

// Выключение порта на PCF8574
void TurnOff(int val)
{
  pcf.digitalWrite(val, HIGH);
}

// Запись статусов емкостей и значений портов вывода (реле) в EEPROM
void WriteStates()
{
  EEPROM.write(0, HotWaterTankIsBlocked);
  EEPROM.write(1, MainTankStatus);
  EEPROM.write(2, StreetTankStatus);
  EEPROM.write(3, HotWaterTankStatus);
  EEPROM.write(5, MainPumpIsBlocked);
}

// Вывод страницы №1
void page1()
{
  int level1, level2;
  char dom[] = {char(180), char(222), char(220), '\0'};
  char str[] = {char(195), char(219), char(216), char(230), char(208), '\0'};

  //graphic commands to redraw the complete screen should be placed here
  u8g.drawRFrame(0, 0, 128, 16, 3);

  u8g.setFont(u8g_font_unifont_0_8);
  u8g.setPrintPos(18, 12);
  u8g.print(dom);

  u8g.setPrintPos(72, 12);
  u8g.print(str);
  u8g.drawVLine(64, 0, 64);

  u8g.setFont(u8g_font_profont22);

  //Уровень домашней емкости (граф.)
  u8g.drawFrame(48, 18, 12, 46);
  level1 = 44 * MainTankPercent / 100;
  if (level1 == 0) {
    level1 = 1;
  }
  u8g.drawBox(50, 20 + (44 - level1), 8, level1);
  u8g.setPrintPos(6, 34);
  u8g.print(MainTankPercent);

  //Уровень уличной емкости (граф.)
  u8g.drawFrame(112, 18, 12, 46);
  level2 = 44 * StreetTankPercent / 100;
  if (level2 == 0) {
    level2 = 1;
  }

  u8g.drawBox(114, 20 + (44 - level2), 8, level2);
  u8g.setPrintPos(68, 34);
  u8g.print(StreetTankPercent);

  /* -----------------------------  Иконка блокировки горячей воды --------------------------------- */
  if ((HotWaterTankIsBlocked == true) && (progress == 1))
  {
    u8g.drawBitmapP(6, 36, 3, 16, ico_hotwater_blocked);
  }
  /* END --------------------------  Иконка блокировки горячей воды ---------------------------- END */

  /* -------------------- Иконка и значение прозрачности воды в уличной емкости ---------------------*/
  if ((StreetTank_WaterQuality < 90) && (progress == 1))
  {
    u8g.drawBitmapP(68, 38, 2, 10, ico_eye);
  }
  else if (StreetTank_WaterQuality >= 90)
  {
    u8g.drawBitmapP(68, 38, 2, 10, ico_eye);
  }

  u8g.setColorIndex(1);
  u8g.setFont(u8g_font_unifont_0_8);
  u8g.setPrintPos(86, 49);
  u8g.print(StreetTank_WaterQuality);
  /* END ---------------- Иконка и значение прозрачности воды в уличной емкости ---------------- END */

  //------------------ Статусы емкостей ------------------------------------------
  if (MainTankStatus == 1) {                                      // При нормальном статусе емкости - ОК
    u8g.drawBitmapP(6, 55, 1, 9, ico_ok_status);
  }

  if (MainTankStatus == 2) {                                      // Анимация при наполнении
    u8g.setFont(u8g_font_unifont_0_8);
    u8g.setPrintPos(14 + (8 * progress), 64);
    u8g.print(">");
  }

  if ((MainTankStatus == 4) && (progress == 1)) {                 // Мигающая иконка при ошибке
    u8g.drawBitmapP(6, 53, 2, 11, ico_err_status);
  }

  if (StreetTankStatus == 1) {              // При нормальном статусе емкости - ОК
    u8g.setFont(u8g_font_unifont_0_8);
    u8g.drawBitmapP(68, 55, 1, 9, ico_ok_status);
  }

  if (StreetTankStatus == 2) {              // При наполнении анимация
    u8g.setFont(u8g_font_unifont_0_8);
    u8g.setPrintPos(68 + (8 * progress), 64);
    u8g.print(">");
  }

  if ((StreetTankStatus == 4) && (progress == 1)) {              // При ошибке анимация
    u8g.drawBitmapP(68, 53, 2, 11, ico_err_status);
  }

  // Отображение стрелок перехода по страницам
  //u8g.drawBitmapP( 117 + progress, 3, 1, 10, right_arrow);
  DrawMenuArrows();
}

// Вывод страницы №2
void page2()
{
  char sauna[] = {char(177), char(208), char(221), char(239), '\0'};

  // Оформление страницы
  u8g.drawRFrame(0, 0, 128, 16, 3);
  u8g.setFont(u8g_font_unifont_0_8);
  u8g.setPrintPos(50, 12);
  u8g.print(sauna);
  //u8g.drawVLine(64, 16, 48);

  u8g.setPrintPos(20, 28);
  u8g.print("100");
  u8g.setPrintPos(20, 46);
  u8g.print(" 50");
  u8g.setPrintPos(20, 64);
  u8g.print("  0");

  // Иконка термометра №1 - тем.воздуха
  u8g.drawBitmapP(62, 18, 2, 16, ico_air_temp);

  // Значение температуры воздуха в парилке
  u8g.setFont(u8g_font_profont22);
  u8g.setPrintPos(80, 32);
  //u8g.print(HotWater_Temp + 53);
  u8g.print(HomeAir_Temp);

  // Иконка термометра №2 - температура воды в баке с горячей водой
  u8g.drawBitmapP(62, 45, 2, 16, ico_water_temp);

  // Значение температуры бака с горячей водой
  u8g.setFont(u8g_font_profont22);
  u8g.setPrintPos(80, 61);
  u8g.print(HotWater_Temp);

  // Уровень емкости с горячей водой (градация 0-50-100%) - всего три уровня
  u8g.drawFrame(48, 18, 12, 46);
  u8g.drawHLine(45, 18, 15);
  u8g.drawHLine(45, 41, 15);
  u8g.drawHLine(45, 63, 15);

  // Прорисовываем уровень емкости
  if (HotWaterTankPercent == 1)
  {
    u8g.drawBox(48, 52, 12, 11);
  }
  if (HotWaterTankPercent == 2)
  {
    u8g.drawBox(48, 30, 12, 33);
  }
  if (HotWaterTankPercent == 3)
  {
    u8g.drawBox(48, 18, 12, 45);
  }

  //--------------------------------- Статус емкости ------------------------------------------
  if (HotWaterTankStatus == 1) {                         // При нормальном статусе емкости - ОК
    u8g.drawBitmapP(6, 55, 1, 9, ico_ok_status);
  }

  if (HotWaterTankStatus == 2) {                         // Анимация при наполнении
    u8g.setFont(u8g_font_unifont_0_8);
    u8g.setPrintPos(8 * progress, 64);
    u8g.print(">");
  }

  if ((HotWaterTankStatus == 4) && (progress == 1)) {    // Мигающая иконка при ошибке
    u8g.drawBitmapP(0, 53, 2, 11, ico_err_status);
  }

  /* -----------------------------  Иконка блокировки горячей воды --------------------------------- */
  if ((HotWaterTankIsBlocked == true) && (progress == 1))
  {
    u8g.drawBitmapP(0, 35, 3, 16, ico_hotwater_blocked);
  }
  /* END --------------------------  Иконка блокировки горячей воды ---------------------------- END */

  /* char text_buffer[16]; // Массив для вывода
    //  u8g.setFont(u8g_font_profont22);
    //  u8g.setPrintPos(26, 36);
    //  sprintf(text_buffer, "#1 %u %", MainTankPercent); //запись в буфер текста и значений
    //  u8g.print(text_buffer);
    //
    //  u8g.setPrintPos(26, 56);
    //  sprintf(text_buffer, "#2 %u %", StreetTankPercent); //запись в буфер текста и значений
    //  u8g.print(text_buffer); */

  // Отображение стрелок перехода по страницам
  //u8g.drawBitmapP( 6 - progress, 3, 1, 10, left_arrow);
  //u8g.drawBitmapP( 117 + progress, 3, 1, 10, right_arrow);
  DrawMenuArrows();
}

// Вывод страницы №3
//void page3()
//{
//  char sauna_temp[] = {char(177), char(208), char(221), char(239), ' ', 't', '1',  '\0'};
//
//  // Оформление страницы
//  u8g.drawRFrame(0, 0, 128, 16, 3);
//  u8g.setFont(u8g_font_unifont_0_8);
//  u8g.setPrintPos(40, 12);
//  u8g.print(sauna_temp);
//
//  // Иконка термометра №1 - тем.воздуха
//  u8g.drawBitmapP(2, 20, 2, 16, ico_air_temp);
//
//  // Значение температуры воздуха в парилке
//  //u8g.setFont(u8g_font_fub14n);
//  u8g.setFont(u8g_font_profont22);
//  u8g.setPrintPos(2, 63);
//  u8g.print(HotWater_Temp + 53);
//
//  u8g.setFont(u8g_font_unifont_0_8);
//  u8g.setPrintPos(20, 28);
//  u8g.print("120");
//  u8g.setPrintPos(20, 46);
//  u8g.print(" 50");
//  u8g.setPrintPos(20, 64);
//  u8g.print("  0");
//
//  // Разметка графика температуры
//  u8g.drawFrame(48, 18, 79, 46);
//  u8g.drawHLine(45, 18, 82);
//  u8g.drawHLine(45, 41, 82);
//  u8g.drawHLine(45, 63, 82);
//
//  u8g.drawLine(50 + x1, 63, 50 + x1, y1);
//
//  // Отображение стрелок перехода по страницам
//  //u8g.drawBitmapP( 6 - progress, 3, 1, 10, left_arrow);
//  //u8g.drawBitmapP( 117 + progress, 3, 1, 10, right_arrow);
//  DrawMenuArrows();
//}

// Вывод страницы SleepPage
//void SleepPage()
//{
//  u8g.setFont(u8g_font_unifont_0_8);
//  u8g.setPrintPos(36, 36);
//  u8g.print(clock.dateFormat("d F Y H:i:s",  dt));
//}

// Счетчик для анимации графических элементов на страницах дисплея
void SystemCounters()
{
  currentMillis = millis();
  //  if (currentMillis - previousMillis >= interval) {
  //    // save the last time you blinked the LED
  //    previousMillis = currentMillis;
  //    x1++;
  //    if (x1 >= 76) {
  //      x1 = 0;
  //    }
  //  }

  if ((currentMillis - previousMillisBLK >= BrightnessInterval) && (SleepMode == false)) {
    previousMillisBLK = currentMillis;
    LED12864_Brightness(SleepBrightness);
    SleepMode = true;
  }

  if (progress < 3 ) {
    progress++;
  }
  else
  {
    progress = 1;
  }
}

// Обработка нажатия кнопок
void ProcessButtons()
{
  int NextPageButtonState = digitalRead(NextPageButtonPin);
  if ((NextPageButtonState == LOW) && (LongTapNext == 0))       // Кнопка нажата (первое нажатие выводит экран из режима сна, последующие переключают страницы)
  {
    if (SleepMode == false)
    {
      LongTapNext = 1;
      CurrentPage++;

      if (CurrentPage > 2) {
        CurrentPage = 1;
      }
      previousMillisBLK = millis();
    }
    else
    {
      LongTapNext = 1;
      SleepMode = false;
      LED12864_Brightness(WorkBrightness);
      previousMillisBLK = millis();
    }
  }

  if ((NextPageButtonState == HIGH) && (LongTapNext == 1)) {        // Кнопка отжата
    LongTapNext = 0;
  }

  //  int PrevPageButtonState = digitalRead(PrevPageButtonPin);
  //  if ((PrevPageButtonState == LOW) && (LongTapPrev == 0))           // Кнопка нажата
  //  {
  //    LongTapPrev = 1;
  //    CurrentPage--;
  //
  //    if (CurrentPage < 1)
  //    {
  //      CurrentPage = 4;
  //    }
  //  }
  //
  //  if ((PrevPageButtonState == HIGH) && (LongTapPrev == 1)) {        // Кнопка отжата
  //    LongTapPrev = 0;
  //  }

}

// Условия для вывода нужной страницы на OLED
void ProcessPages()
{
  if (CurrentPage == 1)
  {
    u8g.sleepOff();
    u8g.firstPage();
    do {
      //Блок отображения страницы №1 на OLED
      page1();

    } while ( u8g.nextPage() );
  }

  if (CurrentPage == 2)
  {
    u8g.sleepOff();
    u8g.firstPage();
    do {
      //Блок отображения страницы №2 на OLED
      page2();

    } while ( u8g.nextPage() );
  }

  //  if (CurrentPage == 3)
  //  {
  //    u8g.sleepOff();
  //    u8g.firstPage();
  //    do {
  //      //Блок отображения страницы №3 на OLED
  //      SleepPage();
  //
  //    } while ( u8g.nextPage() );
  //  }

  //  if (CurrentPage == 4)
  //  {
  //    u8g.setContrast(1);
  //    u8g.firstPage();
  //    do {
  //      //Блок отображения страницы №4 на OLED (Sleep - пока что здесь режим сна)
  //      SleepPage();
  //
  //    } while ( u8g.nextPage() );
  //  }
}

// Отображение на страницах стрелок меню "Влево"/"Вправо"
void DrawMenuArrows()
{
  // Отображение стрелок перехода по страницам
  u8g.drawBitmapP( 6 - progress, 3, 1, 10, left_arrow);
  u8g.drawBitmapP( 117 + progress, 3, 1, 10, right_arrow);
}

/* ----------------------- БЛОК ЛОГИКИ ЕМКОСТЕЙ ---------------------------- */
// Логика уровней/событий домашней емкости
void Logic_MainTank()
{
  // Наполнение
  if (MainTankPercent < MainTank_FillIn)
  {
    if (StreetTankStatus != 4)
    {
      TurnOn(StreetTankRelay);
      MainTankStatus = 2;
    }
    else
    {
      TurnOff(StreetTankRelay);
    }
  }

  // Достигнут максимальный уровень
  if (MainTankPercent >= 100)
  {
    TurnOff(StreetTankRelay);     //Выключаем насос уличной емкости
    MainTankStatus = 1;           //Домашняя емкость - состояние "НОРМА"

    TurnOff(MainTankRelay);       //Выключаем реле блокировки насосной станции
    MainPumpIsBlocked = false;    //Снимаем программную блокировку насосной станции
  }

  // Если уровень ниже уровня блокировки эл.клапана емкости с горячей водой - блокировка клапана
  if (MainTankPercent < MainTank_MinHotWater)
  {
    HotWaterTankIsBlocked = true;
  }
  else if (MainTankPercent > HotWater_Unblock)
  {
    HotWaterTankIsBlocked = false;
  }

  // Если уровень в домашней емкости ниже минимума
  if (MainTankPercent < MainTank_MinBlock)
  {
    TurnOn(MainTankRelay);       //Включаем реле блокировки насосной станции
    MainPumpIsBlocked = true;    //Устанавливаем программную блокировку насосной станции

    if (MainTankStatus != 2 )
    {
      MainTankStatus = 4;
    }
  }
  else
  {
    TurnOff(MainTankRelay);       //Выключаем реле блокировки насосной станции
    MainPumpIsBlocked = false;    //Снимаем программную блокировку насосной станции
    if (MainTankStatus != 2 )
    {
      MainTankStatus = 1;
    }
  }
}

// Логика уровней/событий уличной емкости
void Logic_StreetTank()
{
  // Если уровень уличной емкости достиг критического минимума - запускаем режим наполнения из скважины */
  if (StreetTankPercent < StreetTank_MinBlock)
  {
    //TurnOn(ChinkRelay);  /* Включение реле скважинного насоса */
    StreetTankStatus = 4;
  }
  else
  {
    StreetTankStatus = 1;
  }

  if (StreetTankPercent >= 100 )
  {
    //TurnOff(ChinkRelay);  /* Выключение реле сважинного насоса */
    StreetTankStatus = 1;
  }

  CheckWaterQuality();
}

// Логика уровней/событий домашней емкости
void Logic_HotWaterTank()
{
  // Наполнение
  if (HotWaterTankPercent < 1)
  {
    if (HotWaterTankIsBlocked == false)
    {
      TurnOn(HotWaterTankRelay);      // Открываем электромагнитный клапан
      HotWaterTankStatus = 2;         // Состояние емкости "Наполнение"
    }
    else
    {
      TurnOff(HotWaterTankRelay);     // Закрываем электромагнитный клапан
      HotWaterTankStatus = 4;         // Состояние емкости "Ошибка"
    }
  }

  // Достигнут максимальный уровень
  if (HotWaterTankPercent == 3)
  {
    TurnOff(HotWaterTankRelay);       // Закрываем электромагнитный клапан
    HotWaterTankStatus = 1;           // Состояние емкости "Норма"
  }
}

// Проверка условий прозрачности воды в уличной емкости
void CheckWaterQuality()
{
  if (StreetTank_WaterQuality < 90)
  {
    StreetTankStatus = 4;
  }
  else
  {
    if ((StreetTankStatus != 2) && (StreetTankStatus != 4))
    {
      StreetTankStatus = 1;
    }
  }
}
/* END ------------------- БЛОК ЛОГИКИ ЕМКОСТЕЙ ------------------------ END */

void LED12864_Brightness(int BL_brightness)
{
  analogWrite(BLK, BL_brightness);
}

// Температура в доме
void HomeAirTemp()
{
  HomeAir_Temp = clock.readTemperature();
}

// Температура бака с горячей водой
// также применяем коэффициент сглаживания показаний (как и при работе с ультразвуковыми датчиками) - для устранения "шума"
void HotWaterTemp()
{
  int oldsensorValue = HotWater_Temp;

  if (progress == 1 )
  {
    HotWater_Temp = SlaveHotWater_temp;
    HotWater_Temp = (oldsensorValue * (averageFactor - 1) + HotWater_Temp) / averageFactor;

    //y1 = map(HotWater_Temp + 53, 0, 120, 63, 18);                // Вместо HotWater_Temp + 53 - вывести показания датчика температуры воздуха
    //map - на заметку взять!
  }
}

void i2cReadSlave()
{
  int respVals[5];
  Wire.requestFrom(8, 5);     // Запрашиваем 5 байт из Slave-контроллера по i2c адрес #8
  uint8_t respIoIndex = 0;

  while (Wire.available())
  {
    for (byte r = 0; r < 5; r++)
      if (Wire.available()) {
        respVals[respIoIndex] = (uint8_t)Wire.read();
        respIoIndex++;
      }
      else {
        // log or handle error: "missing read"; if you are not going to do so use r index instead of respIoIndex and delete respoIoIndex from this for loop
        break;
      }
  }

  temp_cm1            = respVals[0];    // Значение уровня в см. для домашней емкости
  temp_cm2            = respVals[1];    // Значение уровня в см. для уличной емкости
  HotWaterTankPercent = respVals[2];    // Значение уровня в баке с горячей водой

  /* Значение температуры с термопары MAX6675 */
  dataA = respVals[3];
  dataB = respVals[4];

  SlaveHotWater_temp  = dataA +  averageFactor + (dataB * 0.1);   // Собираем float из целой и дробной частей

}
