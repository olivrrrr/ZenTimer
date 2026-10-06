#include "DeviceMenu.h"
#include <stdio.h>
namespace {
const uint32_t presets[]={300,600,1200,1800,2700,3600};
void formatDate(char* buffer, size_t size, uint64_t seconds) {
  unsigned days=seconds/86400, year=1970, month=1;
  auto leap=[](unsigned y) { return y%4==0 && (y%100!=0 || y%400==0); };
  while (days>=365u+leap(year)) { days-=365+leap(year); ++year; }
  const unsigned months[]={31,28,31,30,31,30,31,31,30,31,30,31};
  while (month<12 && days>=months[month-1]+unsigned(month==2 && leap(year))) {
    days-=months[month-1]+unsigned(month==2 && leap(year)); ++month;
  }
  snprintf(buffer,size,"%04u-%02u-%02u %02u:%02u UTC",year,month,days+1,unsigned(seconds/3600%24),unsigned(seconds/60%60));
}
}
void DeviceMenu::tap(int16_t x, int16_t y, uint32_t now) {
  if (!open_) return;
  if (y>=200) {
    if (x<80 && (page_==Profiles || page_==History)) { if (index_) --index_; }
    else if (x>=200 && (page_==Profiles || page_==History)) {
      const uint32_t pages=page_==Profiles ? 2 : (store_.journal().sessions()+3)/4;
      if (index_+1<pages) ++index_;
    } else { if (page_==Main) close(); else { page_=Main; index_=0; } }
    dirty_=true; return;
  }
  if (y<42 || y>=194) return;
  const int row=(y-42)/38;
  switch(page_) {
    case Main: page_=row==0?Profiles:row==1?Display:row==2?History:Data; index_=0; break;
    case Profiles: {
      const uint32_t preset=index_*4+row;
      if (preset<6) { store_.beforeAction(now); timer_.setDuration(presets[preset]); store_.afterAction(now); close(); }
      break;
    }
    case Display:
      if (row==0) store_.preferences.time=!store_.preferences.time;
      else if (row==1) store_.preferences.stones=!store_.preferences.stones;
      else if (row==2) store_.preferences.brightness=store_.preferences.brightness==40?70:store_.preferences.brightness==70?100:40;
      else { close(); return; }
      store_.saveSettings(); display_.setPreferences(store_.preferences.time,store_.preferences.stones,store_.preferences.brightness); break;
    case History: {
      JournalRecord r; selected_=index_*4+row;
      if (store_.journal().latest(SessionJournal::End,r,selected_)) page_=Detail;
      break;
    }
    case Data: if (row==0) { page_=History; index_=0; }
      else if (row==1) { if (Serial) store_.startExport(); }
      else if (row==2) page_=Storage;
      else { close(); return; } break;
    default: break;
  }
  dirty_=true;
}
void DeviceMenu::render() {
  display_.pump();
  if (!open_ || !dirty_ || display_.busy()) return;
  char title[40]={}, rows[4][48]={}; const char* footer="Zurueck";
  switch(page_) {
    case Main:
      snprintf(title,sizeof(title),"ZenTimer");
      snprintf(rows[0],48,"Profile"); snprintf(rows[1],48,"Anzeige"); snprintf(rows[2],48,"Gespeicherte Sitzungen"); snprintf(rows[3],48,"Daten / System"); footer="Schliessen"; break;
    case Profiles:
      snprintf(title,sizeof(title),"Profile %lu/2",static_cast<unsigned long>(index_+1));
      for (int i=0;i<4;++i) if (index_*4+i<6) snprintf(rows[i],48,"%lu Minuten",static_cast<unsigned long>(presets[index_*4+i]/60));
      footer="<       Zurueck       >"; break;
    case Display:
      snprintf(title,sizeof(title),"Anzeige");
      snprintf(rows[0],48,"Restzeit: %s",store_.preferences.time?"an":"aus");
      snprintf(rows[1],48,"Steinbild: %s",store_.preferences.stones?"an":"aus");
      snprintf(rows[2],48,"Helligkeit: %u%%",store_.preferences.brightness);
      snprintf(rows[3],48,"Zum Timer"); break;
    case History:
      snprintf(title,sizeof(title),"Sitzungen (%lu)",static_cast<unsigned long>(store_.journal().sessions()));
      for (unsigned i=0;i<4;++i) {
        JournalRecord r;
        if (store_.journal().latest(SessionJournal::End,r,index_*4+i))
          snprintf(rows[i],48,"#%lu %lu:%02lu %s",static_cast<unsigned long>(r.session),static_cast<unsigned long>(r.elapsedMs/60000),static_cast<unsigned long>(r.elapsedMs/1000%60),r.flags==1?"fertig":r.flags==2?"Abbruch":"Strom aus");
      }
      if (!store_.journal().sessions()) snprintf(rows[0],48,"Noch keine Sitzungen");
      footer="<       Zurueck       >"; break;
    case Detail: {
      JournalRecord r;
      if (store_.journal().latest(SessionJournal::End,r,selected_)) {
        snprintf(title,sizeof(title),"Sitzung #%lu",static_cast<unsigned long>(r.session));
        snprintf(rows[0],48,"Dauer: %lu:%02lu",static_cast<unsigned long>(r.elapsedMs/60000),static_cast<unsigned long>(r.elapsedMs/1000%60));
        snprintf(rows[1],48,"Geplant: %lu min",static_cast<unsigned long>(r.plannedSeconds/60));
        snprintf(rows[2],48,"%s",r.flags==1?"Abgeschlossen":r.flags==2?"Abgebrochen":"Stromverlust (Checkpoint)");
        if (r.startedUtc) formatDate(rows[3],48,r.startedUtc);
        else snprintf(rows[3],48,"Datum offen - USB hilft");
      } break;
    }
    case Data:
      snprintf(title,sizeof(title),"Daten / System"); snprintf(rows[0],48,"Sitzungen lesen");
      snprintf(rows[1],48,"USB-Export%s",Serial?"":" (USB fehlt)"); snprintf(rows[2],48,"Speicher / Uhrzeit"); snprintf(rows[3],48,"Zum Timer"); break;
    case Storage:
      snprintf(title,sizeof(title),"Speicher / Uhrzeit"); snprintf(rows[0],48,"%s",store_.statusText());
      snprintf(rows[1],48,"%lu / %lu KiB",static_cast<unsigned long>(store_.journal().used()/1024),static_cast<unsigned long>(store_.journal().capacity()/1024));
      snprintf(rows[2],48,"%s",store_.dateKnown()?"Uhrzeit per USB gesetzt":"Datum offen (keine RTC)"); snprintf(rows[3],48,"Keine automatische Loeschung"); break;
  }
  const char* labels[]={rows[0],rows[1],rows[2],rows[3]};
  if (display_.showMenu(title,labels,footer)) dirty_=false;
}
