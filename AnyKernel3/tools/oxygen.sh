getvalue() { grep "^$1=" "$2" 2>/dev/null | tail -n1 | cut -d= -f2-; }

is_sideload() {
  if getprop sys.usb.state | grep -q "sideload"; then
    return 0
  fi

  return 1
}

keycheck() {
  ui_print " "
  ui_print "Checking volume keys..."
  ui_print "  Press (VOL +) or (VOL -) to confirm!"
  ui_print "  Waiting 10 seconds..."

  local timeout=10
  local start=$(date +%s)

  while true; do
    case $(timeout 0.2 getevent -qlc 1 2>/dev/null | grep -m1 "KEY_VOLUME") in
      *KEY_VOLUMEUP*|*KEY_VOLUMEDOWN*)
        return 0
        break;;
    esac

    local cur=$(date +%s)
    if [[ $(($cur - $start)) -ge $timeout ]]; then
      return 1
    fi
  done
}

# vol_selectopt <message> <1st opt> <2nd opt>
vol_selectopt() {
  ui_print " "
  ui_print "$1"
  ui_print " - (VOL +) $2"
  ui_print " - (VOL -) $3"
  while true; do
    case $(getevent -lt 2>/dev/null | grep -m1 "KEY_VOLUME") in
      *KEY_VOLUMEUP*)
        local selected=$2
        ui_print "$2 Selected!"
        break;;
      *KEY_VOLUMEDOWN*)
        local selected=$3
        ui_print "$3 Selected!"
        break;;
    esac
  done
  ui_print " "
  echo "$selected"
}

### stuff ###

kernvar="default"
susfs=false
dtbo="normal"

var_select() {
  local opt=$(vol_selectopt "Select Variant" "Default (NONROOT)" "KernelSU")
  case "$opt" in
    *Default*|*NONROOT*)
      kernvar="default";;
    *KernelSU*)
      kernvar="ksu";;
  esac
}

susfs_select() {
  if [[ $kernvar == "ksu" ]]; then
    local opt=$(vol_selectopt "SUSFS amogus impostor?" "Default" "SUSFS")
    case "$opt" in
      *Default*)
        susfs="false";;
      *SUSFS*)
        susfs="true";;
    esac
  fi
}

dtbo_select_alioth() {
  opt=$(vol_selectopt "DTBO Type" "Normal" "5K mAh")
  case "$opt" in
    *Normal*)
      dtbo="normal";;
    *5K*)
      dtbo="5k";;
  esac
}

autoinstall() {
  if [[ -f kernel.conf ]]; then
    ui_print "Found kernel.conf inside zip!"
    ui_print "Loading configuration from file..."
    
    local variant=$(getvalue variant kernel.conf)
    if [ ! -z "$variant" ]; then
      kernvar="$variant"
      if [[ $kernvar == "ksu" ]]; then
        ui_print "- Using KernelSU Variant"
      else
        ui_print "- Using NONROOT Variant"
      fi
    fi
    
    local susfs_state=$(getvalue susfs kernel.conf)
    if [ ! -z "$susfs_state" ]; then
      susfs="$susfs_state"
      if [[ $susfs == true ]]; then
        ui_print "- SUSFS"
      fi
    fi
    
    local dtbo_type=$(getvalue dtbo kernel.conf)
    if [ ! -z "$dtbo_type" ]; then
      dtbo="$dtbo_type"
      if [[ $dtbo == "5k" ]] || [[ $dtbo == "5K" ]] || [[ $dtbo == "5000" ]]; then
        ui_print "- Using 5k mAh DTBO"
        dtbo="5k"
      else
        ui_print "- Normal DTBO"
        dtbo="normal"
      fi
    fi
  else
    ui_print "kernel.conf is not found! aborting autoinstall..."
    selectorinstall
  fi
}

selectorinstall() {
  if keycheck; then
    var_select
    susfs_select
    if [[ -f dtbo-5k.img ]]; then
      dtbo_select_alioth
    fi
  else
    ui_print " "
    ui_print "Timed out! using autoinstall..."
    ui_print " "
    autoinstall
  fi
}

startinstall() {
  if is_sideload; then
    ui_print "Sideload install detected!"
    autoinstall
  else
    if [[ "$ZIPFILE" == *auto* ]]; then
      autoinstall
    else
      selectorinstall
    fi
  fi
}

applyconfig() {
  ui_print " "
  ui_print "Applying configuration..."
  if [[ $kernvar == "ksu" ]]; then
    if [[ $susfs == true ]]; then
      mv -f Image-ksu-susfs.gz Image.gz
    else
      mv -f Image-ksu.gz Image.gz
    fi
  fi
  if [[ -f dtbo-5k.img ]]; then
    if [[ $dtbo == "5k" ]]; then
      mv -f dtbo-5k.img dtbo.img;
    fi
  fi
}