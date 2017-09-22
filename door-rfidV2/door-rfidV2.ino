/*
 * Bu kod YTU IEEE Kapı otomasyon sistemi için yazılmıştır.
 * Son güncellenme tarihi: 22 Eylül 2017
 * Son güncelleme değişiklikleri:
 *  Internal Id numarası eklendi.
 *  SD kart başlamadan güvenli modda kapı açma eklendi.
 *
 * Sistem Dosyaları:
 *  adminFile:
 *  Bu dosya kullanıcı Id'lerini saklar
 *  insideFile:
 *  içerideki kişileri tutar.
 *  logFile:
 *  sistem kayıtlarını tutar.
 *
 *  Sistem kullanımı:
 *
 *
 *  Kullanıcı ekleme:
 *    Master Kart 1 kez okutulur. Sistem master moda geçer. Master mod açıkken eklenmek istenen kartlar okutulur. Okutulan kartlar kaydedilir.
 *    İşlem bittiğinde master mod kapatılarak tekrar normal kullanıma dönülür.
 *  Kullanıcı silme:
 *    Master kart 1 kez okutulur. Sistem master moda geçer. Master mod açıkken sililmesini istenen kartlar 3 kez art arda okutulur. Okunan kartlar silinir.
 *    İşlem bittiğinde master mod kapatılarak tekrar normal kullanıma dönülür.
 *  Oturum açma:
 *    Kayıtlı kullanıcı kartını 1 kez okutarak oturum açar.Kapı 5 saniye boyunca açık kalır daha sonra kitlenir. Kayıtlı başka bir kişi kartını tekrar okutana
 *    kadar oturum açık kalır. Oturum açık iken kapıdaki butona basılarak kapı açılabilir. Oturum kapalıyken buton ile kapı açılmaz.
 *  Odayı diğer kullanıcıların girişine kapatma:
 *    Bu özelliği kullanmak için devID.txt dosyasına ID manuel olarak eklenmeli. Yetkili kişi kartını kapıya 6 kez okutursa kapıyı sadece bu kullanıcı açabilir.
 *    Oda, bu kullanıcı tekrar kartını okutana kadar açılmaz.
 *
 *  !!Geliştirici uyarıları!!:
 *    Master kart ID si masterID değişkenine yazılmalı.
 *    Kullanıcı eklerken veya silerken kart eklenme uyarısı geldiğinde master moddan çıkın ve tekrar girin.
 *    Sistem Sd kart okunana kadar başlamaz. Sd kart okunduğunda uyarı ışığı bir süre yanıp söner.
 *    devId dosyasına kişi eklemek için Sd kartın içine devId.txt dosyası açılmalı ve Id numaraları manuel olarak eklenmeli.
 *
 *
 *    Seslerin anlamları:
 *     *    Kısa beep     = 100ms HIGH, 100ms LOW
 *     **   Orta beep     = 300ms HIGH, 300ms LOW
 *     ***  Uzun beep     = 600ms HIGH, 400ms LOW
 *     **** Çok uzun beep = 1sn   HIGH, 600ms LOW
 *
 *    KOMUTLAR:                               SESLER:
 *   Master mod kapalı                        **
 *   Master mod aktif                         ** / **
 *   Oda açıldı                               * / *
 *   Oda kapandı                              * / * / *
 *   Buton ile açıldı                         * / *
 *   Buton ile açılmadı                       *** / *** / *** / ***
 *   izinsiz giriş                            *** / *** / *** / ***
 *   Oda girişleri kapandı                    **** / **** / ****
 *   Oda girişi yasak. Kapı açılmadı.         **** / **** / **** / ****
 *   Oda giriş yasağı kaldırıldı              **** / ****
 *   id eklendi                               ****
 *   id silmek için 2 kere daha okutun        *** / ***
 *   id silmek için 1 kez daha okutun         ***
 *   id silindi                               ** / ** / **
 *
 *
 *   GPL Licence:
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Written by Buğra Tufan, 21 September 2017
 *
 */

#define doorPin 2
#define LED_Pin 10
#define buttonPin A3
#define buzzer 8
#define SD_CS 9
#define RFID_CS 2
#define RFID_RST A2

#include <SD.h>

#include <SPI.h>
#include <RFID.h>

File SD_File;
RFID rfid(RFID_CS,RFID_RST);

const PROGMEM String defaultInternalID  = "19782207182238";//SD kart arızaları için Internal ID numarası. Odayı güvenli moda açar.
String masterID[] = {"13643581254" , "1053726187237"};//Master Card ID'si (Master Kartı değiştirmek için sadece bu id'nin değişmesi yeterli
boolean masterAuthorization = false;// Master kartın okunduğunu ve kişi ekleme çıkarma moduna girdiğini gösterir
boolean doorState = false;// Kapının bir kişi tarafından açıldığını ve kapının kartsız açılabildiğini gösterir
boolean newId = false; //Kartın sürekli okunmaması için gereken bir değişken
String lastID = "";// Okunan karttan önceki kartın numarasıdır. Kartın art arda okunup okunmadığını anlamak için vardır.
int idCounter = 0;// Bir kartın art arda kaç defa okunduğunu sayar
int adminCounter = 0;//İçerideki yetkili kişi sayısı

String insideFile = "inside.txt";
String adminFile = "admin.txt";

void setup() {
  pinMode(doorPin,OUTPUT);
  pinMode(LED_Pin,OUTPUT);
  pinMode(buzzer,OUTPUT);
  digitalWrite(doorPin,HIGH);
  Serial.begin(9600);
  saveLog("","Sd bekleniyor");
  SPI.begin();
  rfid.init();
  while(SD.begin(9)==0){
    String ID = readRFID();
    if(ID.length() == 0 ){
      if(ID == defaultInternalID){
        saveLog(ID,"Sd kart baslatilamadi.");
        saveLog(ID,"Oda guvenli modda acıldı");
        alertSound(2, 100, 100);
        unlockDoor();
      }
    }
  }
  saveLog("","Sistem baslatildi");
  for(byte a = 0; a<3; a++){
    digitalWrite(LED_Pin,HIGH);
    delay(100);
    digitalWrite(LED_Pin,LOW);
    delay(100);
  }
  //Gerekli olan dosyaları düzenler.
  SD.remove(insideFile);//Son girişleri siler odayı kapalı modda açar.
  SD_File = SD.open(adminFile, FILE_WRITE);
  SD_File.close();
  SD_File = SD.open(insideFile, FILE_WRITE);
  SD_File.close();
  SD_File = SD.open(logFile, FILE_WRITE);
  SD_File.close();
}

void loop() {
   String ID = readRFID();// Kartı okutan kişinin ID'si
   digitalWrite(LED_Pin, doorState);

   boolean buttonState = digitalRead(buttonPin);
   if(buttonState){
      if(adminCounter > 0){
        saveLog("Buton:","buton ile giris");
        alertSound(2,100,100);
        unlockDoor();
      }
      else{
        alertSound(4,200,200);
      }
   }


  if(ID.length() == 0 ){
    newId = true;
  }
  if(ID.length()>0 && newId == true){
    newId = false;
    if(ID.equals(lastID)){
      idCounter++;
    }
    else{
      idCounter = 0;
    }
    lastID = ID;
    if( inArray(ID, masterID, sizeof(masterID) ) ){
      if(!masterAuthorization){
        saveLog(ID, "master mode aktif");
        alertSound(2,300,300);
      }
      else if(masterAuthorization){
        saveLog(ID, "master mode kapali");
        alertSound(1,300,300);
      }
      masterAuthorization = !masterAuthorization;
    }
    else{
      if( ID == defaultInternalID){
        saveLog(ID,"Oda Guvenli Modda acıldı");
        alertSound(2,100,100);
        unlockDoor();
      }
      else if(masterAuthorization){
          if(!compare(adminFile,ID)){
            addLine(adminFile,ID);
            printFile(adminFile);
            saveLog("ID", "ekleme isteği");
            alertSound(1,1000,0);
            idCounter = 0;
          }
          else{
            switch(idCounter){
              case 0:
                saveLog(ID, "Bu kart zaten kayitli");
                saveLog(ID, "Silmek icin 2 kez daha okutun");
                alertSound(2,600,400);
                break;
              case 1:
                saveLog(ID, "Bu kart zaten kayitli");
                saveLog(ID,"Silmek icin 1 kez daha okutun");
                alertSound(1,600,0);
                break;
              case 2:
                saveLog(ID,"Silme istegi");
                idCounter = 0;
                deleteLine(adminFile,ID);
                printFile(adminFile);
                break;
            }

          }
      }
      else if(compare(adminFile, ID)){
        if(compare(insideFile, ID)){
          deleteLine(insideFile,ID);
          adminCounter--;
          alertSound(3,100,100);
        }
        else{
          addLine(insideFile,ID);
          adminCounter++;
          alertSound(2,100,100);
        }
        saveLog(ID, "kapi acildi");
        printFile(insideFile);
        unlockDoor();

      }
      else{
        if(adminCounter>0){
          saveLog(ID,"buton gibi kartla acildi");
          alertSound(2,100,100);
          unlockDoor();
        }
        else{
          if(idCounter>3 && idCounter<=7){
            saveLog(ID, "Kaybol burdan gozum gormesin");
            alertSound(4,1000,400);
          }
          else if(idCounter>7){
            saveLog(ID, "Burayi terk et");
            alertSound(4,2000,500);
          }
          else{
            saveLog(ID, "Izinsiz giris");
            alertSound(4,600,400);
          }
        }
      }
    }
  }
}


String readRFID(){
  String ID = "";
  if(rfid.isCard()){
    if (rfid.readCardSerial()) {
      for( byte a = 0; a<5; a++){
        ID += String(rfid.serNum[a]);
      }
    }
  }
  rfid.halt();
  return ID;
}

void unlockDoor(){
  digitalWrite(doorPin,LOW);
  for(byte a = 0; a<25; a++){
    digitalWrite(LED_Pin,HIGH);
    delay(50);
    digitalWrite(LED_Pin,LOW);
    delay(50);
  }
  digitalWrite(doorPin,HIGH);
}
void saveLog( String ID , String logString ){
  Serial.print(ID);
  Serial.println(" "+logString);
  //SD Karta Log Kaydı tutmak içindir.
  /*SD_File = SD.open(logFile, FILE_WRITE);
  if (SD_File) {
    SD_File.println(ID+""+logString);
    SD_File.close();
  }*/
}

boolean addLine(String file, String ID ){
  SD_File = SD.open(file, FILE_WRITE);
  if (SD_File) {
    SD_File.println(ID);
    saveLog(ID, "eklendi");
    SD_File.close();
    return true;
  }
  else {
    saveLog(file , "dosya acilmadi");
  }
  SD_File.close();
  return false;
}

boolean deleteLine( String file, String ID ){
  String newData = "";
  SD_File = SD.open(file);
  if (SD_File) {
    while (SD_File.available()) {
      String line = SD_File.readStringUntil('\n');
      line.replace("\r", "");
      line.replace("\n", "");
        if( !ID.equals(line) ){
          newData = line+"\n";
        }
    }
    SD_File.close();
    SD.remove(file);
    SD_File = SD.open(file, FILE_WRITE);
    if (SD_File) {
      SD_File.print(newData);
      SD_File.close();
      return true;
    }
    else {
      saveLog(file , "dosya acilmadi");
    }
  }
  else {
    saveLog(file , "dosya acilmadi");
  }
  SD_File.close();
  return false;
}



boolean compare(String file, String ID ){
  SD_File = SD.open(file);
  if (SD_File) {
    while (SD_File.available()) {
      String line = SD_File.readStringUntil('\n');
      line.replace("\r", "");
      line.replace("\n", "");
        if( (ID ).equals(line) ){
          SD_File.close();
          Serial.println("True");
          return true;
        }
    }
    SD_File.close();
    return false;
  }
  else {
    saveLog(file , "dosya acilmadi");
  }
  SD_File.close();
  return false;
}




boolean printFile(String file){
  SD_File = SD.open(file);
  if (SD_File) {
    Serial.println(String("***************") + " File: " + file + String(" *********************"));
    while (SD_File.available()) {
      String line = SD_File.readStringUntil('\n');
      line.replace("\r", "");
      line.replace("\n", "");
      Serial.println(line);
    }
    SD_File.close();
    Serial.println("*************** END OF FILE *********************");
  }
}


void alertSound(byte amount, int delayTime_H,int delayTime_L){
  for( byte a = 0; a<amount; a++){
    digitalWrite(buzzer,HIGH);
    digitalWrite(LED_Pin,HIGH);
    delay(delayTime_H);
    digitalWrite(buzzer,LOW);
    digitalWrite(LED_Pin,LOW);
    delay(delayTime_L);
  }
}
boolean inArray(String data, String* arr,int sizeOf){
  for( int a = 0; a< sizeOf; a++){
    if( arr[a] == data){
      return true;
    }
  }
  return false;
}
