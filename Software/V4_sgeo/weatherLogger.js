// JavaScript Document
//  $Id: weatherLogger.js 3872 2023-08-25 16:38:47Z SGEO $
/* jscript script for logging weather information
   Data gathered from ASCOM Observing Conditions Hub (OCH)
   OCH gets data from AM WeatherBox2 Alpaca device  and various web services
   Data logged to C:\Users\<user>\Documents\ACP Astronomy\Logs\weather\<datenite>.log
   Windows Task Scheduler calls this every 5 min.
*/

var OCH;
var SMon;
var FSO;

// fetch data from OCH
var OCH = new ActiveXObject("ASCOM.OCH.ObservingConditions");
OCH.Connected= true;
var SMon = new ActiveXObject("ASCOM.Environment.SafetyMonitor");  // nb: not the hub
SMon.Connected= true;
FSO = new ActiveXObject("Scripting.FileSystemObject");
//WScript.Sleep(5000);

// get date of last boltwood data
var ts= FSO.OpenTextFile("boltwood.txt", 1); 
var lastdate= FSO.GetFile("boltwood.txt").DateLastModified;
//WScript.Echo(lastdate);
ts.Close();

var od= getOCHdataWBox2(OCH, SMon);
od.since= (od.date - lastdate)/1000; 
var bolt= makeBWoodstring(od);
WScript.Echo(bolt);

OCH= null;
SMon= null;
 
// save boltwood file
var fhb= FSO.OpenTextFile("boltwood.txt", 2, true ); // write, create if new  
fhb.WriteLine(bolt);
fhb.Close();
// save file to datenite log
var dn= od.date;
if(dn.getHours() < 12) dn.setDate(dn.getDate()- 1); // datenite is yesterday
var dnfn= (''+ dn.getFullYear()- 2000)+ pad(''+ (1+ dn.getMonth())) + pad(''+dn.getDate() )+ '.log';
//WScript.Echo(dnfn);   
var docdir= new ActiveXObject("WScript.Shell").SpecialFolders("MyDocuments");
var logfile= docdir+ '\\ACP Astronomy\\Logs\\weather\\'+ dnfn;
var newlog= FSO.FileExists(logfile);
var fh= FSO.OpenTextFile(logfile, 8, true ); // append, create if new 
if (!newlog) { // new file; add header
   fh.WriteLine("#TYPE=WEATHER_LOG");
   fh.WriteLine("# from scheduled task running c:\\Data\\Plans\\Scripts\\weatherLogger.js");
   fh.WriteLine("# Date     Time     T V   SkyT   AmbT   SenT   Wind Hum  DewPt Hea R W Since  Now() Days  c w r d C A "); //+ Cloud%  barometer"); 
} 
fh.WriteLine(bolt);
fh.Close();
FSO= null;



function getOCHdataWBox2(OCH, SMon) {
   var od= {};

   od.date= new Date();
   //od.AveragePeriod= OCH.AveragePeriod; // hrs
   od.CloudCover= OCH.CloudCover; // 0.0-100.0 %
   od.DewPoint= OCH.DewPoint; // C
   od.TempUnits= 'C';
   od.Humidity= OCH.Humidity; // 0.0-100.0 %
   //WScript.Echo(od.Humidity);
   if (od.Humidity == null)
       od.Humidity= OCH.Humidity; // 0.0-100.0 %
   od.Pressure= OCH.Pressure; // hPa  local
   od.RainRate= OCH.RainRate; // 0/10   dry/wet
   //od.SkyBrightness= OCH.SkyBrightness; // lux
   //od.SkyQuality= OCH.SkyQuality; // magnitudes per square arc second
   od.SkyTemperature= OCH.SkyTemperature; // C
   //od.StarFWHM= OCH.StarFWHM; // arc seconds
   od.Temperature= OCH.Temperature; // C
   od.WindDirection= OCH.WindDirection; // 0.0-360.0, 360 is North, 0.0 is no wind
   //od.WindGust= OCH.WindGust; // peak 3s windspeed over last 2 min 
   od.WindSpeed= OCH.WindSpeed; // m/s
   od.WindSpeedUnits= 'm';
   od.Pressure= od.Pressure; 
   
   od.issafe= SMon.issafe;
   return od;
}

function makeBWoodstring(od) {
    var r;
    r= ISOdate(od.date);
    r+= ' '+ od.TempUnits;
    r+= ' '+ od.WindSpeedUnits;
    r+= ('     '+ formatNumber(od.SkyTemperature, 1)).slice(-7);
    r+= ('     '+ formatNumber(od.Temperature, 1)).slice(-7);
    r+= ('     '+ formatNumber(od.Temperature, 1)).slice(-7);
    r+= ('     '+ formatNumber(od.WindSpeed, 1)).slice(-7); 
    r+= ('     '+ formatNumber(od.Humidity, 0)).slice(-4); 
    r+= ('     '+ formatNumber(od.DewPoint, 1)).slice(-7);
    r+= '   0'; // heater power
    r+= (0== od.RainRate? ' 0': ' 2'); // rain
    r+= (0== od.RainRate? ' 0': ' 2'); // wet
    //r+= ' 00030'; // 10s    unverified. Not true
    r+= ' '+ ('000000'+ formatNumber(od.since, 0)).slice(-5);
    r+= ' dddddd.ddddd'; // Now in VB6 ?   deprecated
    if (od.CloudCondition > 80) r+= ' 3'; // VeryCloudy
    else if (od.CloudCondition > 20) r+= ' 2'; // Cloudy
    else r+= ' 1'; // Clear
    if (od.WindSpeed > 30) r+= ' 3'; // VeryWindy
    else if (od.WindSpeed > 10) r+= ' 2'; // Windy
    else r+= ' 1'; // Calm
    r+= ' 0'; // rain condition
    r+= ' 0'; // daylight condition
    r+= ' 0'; // roof close
    r+= (od.issafe== 1)? ' 0': ' 1';
    // extra info:  (not part of the Boltwood spec; and not compatible with some software)  
    //r+= ('     '+ formatNumber(od.CloudCover, 0)).slice(-4); 
    //r+= ('     '+ formatNumber(od.Pressure, 0)).slice(-6); 
    
    return r; 
}



/* boltwood one line file format:
This recommended format gives access to all of the data Cloud Sensor II can provide.  
The data is similar to the display fields in the Clarity II window.  The format has been split across 
two lines to make it fit on this page: 
Date       Time        T V   SkyT   AmbT   SenT   Wind Hum  DewPt Hea R W Since  Now() Day's c w r d C A 
2005-06-03 02:07:23.34 C K  -28.5   18.7   22.5   45.3  75   10.3   3 0 0 00004 038506.08846 1 2 1 0 0 0 
2019-10-31_23:21:56.7 _F_M_  60.4_  69.3   73.6_  51.0_ 82_  63.5_ 50_ _ _

The header line is here just for illustration.  It does not actually appear anywhere. The fields mean:     
Heading Col’s    Meaning     
Date          1-10 local date yyyy-mm-dd     
Time         12-22 local time hh:mm:ss.ss (24 hour clock)     
T               24 temperature units displayed and in this data, 'C' for Celsius or 'F' for Fahrenheit     
V               26 wind velocity units displayed and in this data, ‘K’ for km/hr or ‘M’ for mph or 'm' for m/s     
SkyT         28-33 sky-ambient temperature, 999. for saturated hot, -999. for saturated cold, or –998. for wet     
AmbT         35-40 ambient temperature     
SenT         41-47 sensor case temperature, 999. for saturated hot, -999. for saturated cold.  Neither saturated condition should ever occur.     
Wind         49-54 wind speed or: -1. if still heating up,  -2. if wet,  -3. if the A/D from the wind probe is bad (firmware <V56 only) ,  -4. if the probe is not heating (a failure condition),  -5. if the A/D from the wind probe is low (shorted, a failure condition) (firmware >=V56 only),  -6. if the A/D from the wind probe is high (no probe plugged in or a failure) (firmware >=V56 only).     
Hum          56-58 relative humidity in %     
DewPt        60-65 dew point temperature     
Hea          67-69 heater setting in %     
R               71 rain flag, =0 for dry, =1 for rain in the last minute, =2 for rain right now     
W               73 wet flag, =0 for dry, =1 for wet in the last minute, =2 for wet right now     
Since        75-79 seconds since the last valid data     
Now() Day's  81-92 date/time given as the VB6 Now() function result (in days) when Clarity II last wrote this file    deprecated   
c               94 cloud condition (see the Cloudcond enum in section 20) 
w               96 wind condition (see the Windcond enum in section 20)     
r               98 rain condition (see the Raincond enum in section 20)     
d              100 daylight condition (see the Daycond enum in section 20)     
C              102 roof close, =0 not requested, =1 if roof close was requested on this cycle     
A              104 alert, =0 when not alerting, =1 when alerting

Public Enum CloudCond    
    cloudUnknown = 0    
    'Those below are based upon thresholds set in the setup window.    
    cloudClear = 1 
    cloudCloudy = 2    
    cloudVeryCloudy = 3 
End Enum 
Public Enum WindCond    
    windUnknown = 0    
    'Those below are based upon thresholds set in the setup window.    
    windCalm = 1    
    windWindy = 2    
    windVeryWindy = 3 
End Enum Public 
Enum RainCond    
    rainUnknown = 0    
    'Those below are based upon thresholds set in the setup window.    
    rainDry = 1    
    rainWet = 2  'sensor has water on it    
    rainRain = 3 'falling rain drops detected 
End Enum Public 
Enum DayCond    
    dayUnknown = 0    
    'Below are based upon thresholds set in the setup window.    
    dayDark = 1    
    dayLight = 2    
    dayVeryLight = 3
    
End Enum 

*/

function formatNumber(number, dig) {
    //WScript.Echo(number); WScript.Echo(dig);
    number = number.toFixed(dig) + '';
    x = number.split('.');
    x1 = x[0];
    x2 = x.length > 1 ? '.' + x[1] : '';
    var rgx = /(\d+)(\d{3})/;
    //while (rgx.test(x1)) {         x1 = x1.replace(rgx, '$1' + ',' + '$2');     }
    return x1 + x2;
}

function pad(number) {
    if (number < 10) {
      return '0' + number;
    }
      return number;
}

function ISOdate(dd) { // dd is from Date()
    return dd.getFullYear() +
        '-' + pad(dd.getMonth() + 1) +
        '-' + pad(dd.getDate()) +
        ' ' + pad(dd.getHours()) +
        ':' + pad(dd.getMinutes()) +
        ':' + pad(dd.getSeconds()) 
        //'.' + (dd.getUTCMilliseconds() / 1000).toFixed(3).slice(2, 5) +
        //'Z'
        ;

  
}
