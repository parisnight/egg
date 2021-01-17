#!/bin/sh

#  m, a poor man's shell menu

saslauthd_start() {
  # If saslauthd is not running, start it:
  if [ ! -r /var/state/saslauthd/saslauthd.pid ]; then
    # Use shadow authentication by default on Slackware:
    echo "Starting SASL authentication daemon:  /usr/sbin/saslauthd -a shadow"
    /usr/sbin/saslauthd -a shadow
  fi
}

saslauthd_stop() {
  kill `cat /var/state/saslauthd/saslauthd.pid 2> /dev/null` 2> /dev/null
  sleep 1
}

saslauthd_restart() {
  saslauthd_stop
  saslauthd_start
}

case "$1" in
'fi')
  /usr/firefox/firefox -width 900 -height 800 &
  ;;
'sl')
  sudo pm-suspend & ;;
'em') emacs -nw ;;
'st') $HOME/Downloads/st-0.8.4/st & ;;
'sy') sylpheed & ;;
'we') 
  cd /tmp
  rm gerald* miz*
  #wget ftp://tgftp.nws.noaa.gov/data/forecasts/discussion/gerald_r_ford.txt
  wget ftp://tgftp.nws.noaa.gov/data/forecasts/state/mi/miz015.txt
  egrep -A3 "alamazoo|etroit" miz*
  ;;
'gp') wget -qO- 192.168.1.1 |grep ipinfo ;;
'restart')
  saslauthd_restart
  ;;
'in')
  # nut no tabs   i2 indent 2 spaces
  indent -o $2ind -kr -nut -i2 $2 ;;
'sg')
  # m sg guile "(atan 0.4)"
  #screen -S 1493.tty1.darkstar -p $2 -X stuff "$3"`echo -ne '\015'`
  screen -S $STY -p $2 -X stuff "$3"`echo -ne '\015'` ;;
'gi')
  #  git config --global credential.helper cache
  git add -u
  git commit -m "$2"
  git push;;
'xt')
  #xterm -rv -fa Monospace -fs 14
  #xterm -rv -fn -misc-fixed-medium-r-normal--20-200-75-75-c-100-iso8859-1
  xterm -rv -fn -k-knxt-medium-r-normal--20-200-72-72-c-100-iso10646-1
  ;;
'af')
  wget --no-check-certificate -O - https://freedns.afraid.org/dynamic/update.php?S1E3V05Ub1pXUWlSd0dyM2Y5Y3Y6MTkyNTMwNzk=  >> /tmp/freedns_parisnight_mooo_com.log 2>&1  ;;
'cu') rm *.dvi *.aux *.log *.fas *.lib *.x86f *\~ \#*  ;;

*)
  echo "usage $0 command"
esac
