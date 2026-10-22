#include <SPI.h>                  // Incluye la librería para la comunicación SPI con el lector RFID
#include <MFRC522.h>              // Incluye la librería que controla el módulo RFID MFRC522
#include <Servo.h>                // Incluye la librería para controlar el servomotor
#include <LiquidCrystal_I2C.h>    // Incluye la librería para controlar la pantalla LCD mediante I2C


MFRC522 rfid(10, 9);                    // Lector RFID: SS = pin 10, RST = pin 9
Servo servo;                            // Servo de la cerradura
LiquidCrystal_I2C lcd(0x3F, 16, 2);     // Pantalla I2C: direccion 0x3F, 16 columnas, 2 filas
const byte BUZZER = 7;                  // Pin del buzzer
const byte UID[] = {0xCD, 0x39, 0x3E, 0x02};  // UID de la tarjeta autorizada

// Borra la pantalla y escribe dos lineas de texto
void pantalla(const __FlashStringHelper* a, const __FlashStringHelper* b = F("")) {
  lcd.clear();
  lcd.print(a);                         // Linea 1
  lcd.setCursor(0, 1);                  // Pasa a la linea 2
  lcd.print(b);
}

// Mensaje de espera
void inicio() { pantalla(F("CERRADURA RFID"), F("Acerque tarjeta")); }

// Suena el buzzer a una frecuencia (Hz) durante d milisegundos
void beep(int f, int d) { tone(BUZZER, f); delay(d); noTone(BUZZER); }

// Sonido de aceptado: dos pitidos cortos y graves
void sonidoAceptado() {
  beep(800, 100);                       // Mas bajo = mas grave
  delay(70);
  beep(1000, 120);
}

void setup() {
  Serial.begin(9600);                   // Monitor serial para ver los UID
  SPI.begin();                          // Inicia el bus SPI del lector
  rfid.PCD_Init();                      // Inicia el lector RFID
  lcd.init();                           // Inicia la pantalla
  lcd.backlight();                      // Enciende la luz de fondo
  pinMode(BUZZER, OUTPUT);              // Buzzer como salida
  servo.attach(2); servo.write(0);      // Deja la cerradura cerrada al arrancar
  delay(500);                           // Espera a que el servo llegue
  servo.detach();                       // Lo suelta para evitar vibracion
  inicio();
}

void loop() {
  // Si no hay tarjeta nueva o no se puede leer, vuelve a intentar
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  // Muestra el UID de la tarjeta en el monitor serial
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(' ');
  }
  Serial.println();

  // Compara el tamano y los bytes del UID con la tarjeta autorizada
  if (rfid.uid.size == sizeof(UID) && !memcmp(rfid.uid.uidByte, UID, sizeof(UID))) {
    pantalla(F("Acceso correcto"), F("Bienvenido!"));
    sonidoAceptado();                   // Dos pitidos graves
    servo.attach(2);
    servo.write(90);                    // Abre la cerradura
    pantalla(F("Puerta abierta"));
    delay(5000);                        // Mantiene abierta 5 segundos
    servo.write(0);                     // Cierra la cerradura
    delay(600);                         // Espera a que el servo llegue
    servo.detach();
    pantalla(F("Puerta cerrada"));
  } else {
    pantalla(F("Acceso denegado"), F("Tarjeta invalida"));
    beep(300, 400);                     // Un pitido grave largo
  }

  delay(1500);                          // Deja el mensaje visible un momento
  rfid.PICC_HaltA();                    // Termina la comunicacion con la tarjeta
  rfid.PCD_StopCrypto1();
  inicio();                             // Vuelve a la pantalla de espera
}