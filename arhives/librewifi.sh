#!/bin/bash

KEY='9879683003' # identifiant freewifi, aller sur wifi.free.fr si vous ne l'avez pas encore
PASSWORD='wificdp1' # mot de passe freewifi

# interface wifi utilisee pour la connexion freewifi
#IFACE=wlan1
IFACE=wlan0

WIFIESSID="FreeWifi"

# hote de reference pour verifier la connexion reseau
HOSTREF=""
#HOSTREF="wifi.free.fr"
#HOSTREF="localhost"

# authentification toutes les 30 mn (1800s)
AUTH_INTERVAL=1800

LOGFILE="/tmp/$0.log"

# attend que l'ip d'une interface soit fixee
# param1 : nom de l'interface
# param2 : duree max d'attente en nombre d'essai. Un essai a lieu au mieux toutes les secondes
waitipaddr()
{
   WAITIPADDR_IFACE=$1
   WAITIPADDR_LIMIT=$2
   WAITIPADDR_COUNTER=0
   while [ true ]
   do
      WAITIPADDR_IP=`ifconfig $WAITIPADDR_IFACE | grep "inet addr" | cut -d ':' -f 2 | cut -d ' ' -f 1`
      if [ -n "$WAITIPADDR_IP" ]
      then
         echo 0
         return 0
      else
         WAITIPADDR_COUNTER=`expr $WAITIPADDR_COUNTER + 1`
         if [ $WAITIPADDR_COUNTER -gt $WAITIPADDR_LIMIT ]
         then
            echo 1
            return 1
         fi
         sleep 1
      fi
   done
}


# attend que l'essid d'un reseau wifi soit disponible sur une interface
# param1 : nom de l'interface
# param2 : nom du reseau wifi
waitessid()
{
   WAITESSID_IFACE="$1"
   WAITESSID_ESSID="$2"
   while true
   do
      WAITSSID_LIST=`iwlist "$WAITESSID_IFACE" scan | grep -e "ESSID:" -e "Quality" | while read line; do ESSID=$line; read line; QUALITY=$line; echo $ESSID $QUALITY; done | grep "\"$WAITESSID_ESSID\""`
      if [ -n "$WAITSSID_LIST" ]
      then
         return 0
      fi
      sleep 5
   done
}


# verification de l'accessibilite reseau d'un hote
# param1 : nom ou adresse ip de l'hote
isreachable()
{
   if [ -n "$1" ]
   then
      ISREACHABLE_RET=0
      ping -n -q -c1 "$1" > /dev/null 2>&1
      ISREACHABLE_RET=$?
      echo $ISREACHABLE_RET
      return $ISREACHABLE_RET
   else
      echo 0
      return 0
   fi
}


# attend qu'un hote soit accessible
# param1 : nom ou adresse ip de l'hote
# param2 : duree max d'attente en seconde
waitreachable()
{
   if [ $# -ne 2 ]
   then
      echo 0
      return 0
   fi

   WAITREACHABLE_HOST=$1
   WAITREACHABLE_INTERVAL=$2
   WAITRECHABLE_COUNTER=0

   WAITREACHABLE_RET=1
   while [ $WAITREACHABLE_RET -ne 0 ]
   do
      ping -n -q -c1 "$WAITREACHABLE_HOST" > /dev/null 2>&1
      WAITREACHABLE_RET=$?
      if [ $WAITREACHABLE_RET -ne 0 ]
      then
         WAITREACHABLE_COUNTER=`expr $WAITREACHABLE_COUNTER + 1`
         if [ $WAITREACHABLE_COUNTER -gt $WAITREACHABLE_INTERVAL ]
         then
            echo 1
            return $WAITREACHABLE_RET
         fi
         sleep 1
      fi
   done
   echo 0
   return 0
}



wifistatus()
{
# format du pseudo fichier /proc/net/wireless
#Inter-| sta-|   Quality        |   Discarded packets               | Missed | WE
# face | tus | link level noise |  nwid  crypt   frag  retry   misc | beacon | 22
# wlan0: 0000    0     0     0        0      0      0      0      0        0
# wlan1: 0000   44.  -75.  -256.       0      0      0      0      0        0
   WIFISTATUS_IFACE=$1
   cat "/proc/net/wireless" | while read line
   do
      set $line
      if [ "$1" == "$WIFISTATUS_IFACE"":" ]
      then
         WIFISTATUS_QUALITY=`expr $3 : "\(.*\)\."`
         WIFISTATUS_LEVEL=`expr $4 : "\(.*\)\."`
         echo "Q=$3 LVL=$4"
         echo "QUAL=$WIFISTATUS_QUALITY LVL=$WIFISTATUS_LEVEL"
         lcd_echo -y 2 "QUAL=$WIFISTATUS_QUALITY LVL=$WIFISTATUS_LEVEL     "
      fi
   done
   return 0
}


SLEEP_DELAY=5
INTERVAL1=`expr $AUTH_INTERVAL / $SLEEP_DELAY`
INTERVAL2=`expr $INTERVAL1 '*' 2`

#_echo "(GREEN LED OFF, ORANGE LED OFF, RED LED OFF)"
lcd_echo -r --y=1 " ### libre wifi ### "
sleep 3
while true
do
   ifdown $IFACE >> $LOGFILE 2>&1

#   echo "(GREEN LED OFF, ORANGE LED OFF, RED LED ON)"
   echo "NO CONNECTION"
   lcd_echo -e "NO CONNECTION"
   lcd_echo --y=1 "wait freewifi"
   waitessid "$IFACE" "$WIFIESSID"
   lcd_echo --y=1 "             "

#   lcd_echo -e "(GREEN LED OFF, ORANGE LED ON, RED LED OFF)"
   echo "CONNECTING ..."
   lcd_echo -e "CONNECTING ..."
   lcd_echo --y=1 "ifup $IFACE"
   ifup $IFACE >> $LOGFILE 2>&1
   RET=$?
   lcd_echo --y=1 "          "
   if [ $RET -eq 0 ]
   then
      echo "wait ip addr"
      lcd_echo --y=1 "wait ip addr"
      RET=`waitipaddr $IFACE 5`
      lcd_echo --y=1 "            "
      if [ $RET -eq 0 ]
      then
         RET=`waitreachable $HOSTREF 5`
         if [ $RET -eq 0 ]
         then
#            echo "(GREEN LED ON, ORANGE LED OFF, RED LED OFF)"
            echo "CONNECTED"
            lcd_echo -e "CONNECTED"
            iptables -t nat -A POSTROUTING -o $IFACE -j MASQUERADE >> $LOGFILE 2>&1
            COUNTER=`expr $INTERVAL1 + 1`
            CONTINUE=1
            while [ $CONTINUE -eq 1 ]
            do
               if [ $COUNTER -gt $INTERVAL1 ]
               then
                  echo "auth."
                  lcd_echo --y=1 "auth."
                  wget --quiet --no-check-certificate --post-data\
                  'login='"$KEY"'&password='"$PASSWORD"'&submit=Valider'\
                  'https://wifi.free.fr/Auth' -O- >> $LOGFILE 2>&1
                  RET=$?
                  lcd_echo --y=1 "     "
                  if [ $RET -eq 0 ]
                  then
                     echo "auth. done"
                     lcd_echo --y=1 "auth. done"
                     sleep 3
                     lcd_echo --y=1 "          "
                     COUNTER=0
                  else
                     echo "auth. failed"
                     lcd_echo --y=1 "auth. failed"
                     sleep 3
                     lcd_echo --y=1 "            "
                  fi
               else
                  COUNTER=`expr $COUNTER + 1`
               fi

               sleep $SLEEP_DELAY

               RET=`waitipaddr $IFACE 5`
               if [ $RET -ne 0 ]
               then
#                  echo "(GREEN LED OFF, ORANGE LED OFF, RED LED ON)"
                  echo "CONNECTION LOST"
                  lcd_echo -e "CONNECTION LOST"
                  CONTINUE=0
               else
                  wifistatus $IFACE
               fi
            done
         else
            echo "$HOSTREF NOT REACHABLE"
         fi
      else
         echo "no ip addr"
         lcd_echo --y=1 "no ip addr"
      fi
   fi
done

