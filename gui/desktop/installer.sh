#!/bin/sh


# Base
DESTDIR=
srcdir=.
datadir="/usr/share"
applications="$datadir/applications"
icons="$datadir/icons"
pixmaps="$datadir/pixmaps"
mime_info="$datadir/mime-info"
application_registry="$datadir/application-registry"
applnk="$datadir/applnk"
mimelnk="$datadir/mimelnk"
menutype=applnk
onlyexistingdirs=no
dryrun=no

# Autodetect

desktop_file_install=`which desktop-file-install 2>/dev/null`
if [ -x "$desktop_file_install" ] ; then
    menutype=redhat
elif [ x${XDG_CONFIG_DIRS} != x ] ; then
    menudtype=xdg
elif [ -r /etc/xdg/menus/applications.menu ] ; then
    menutype=xdg
fi

kdeconfig=`which kde-config 2>/dev/null`
if [ -x "$kdeconfig" ]
then
    d=`$kdeconfig --path mime | sed -e 's/^.*://g'`
    test -d "$d" && dir_mimelnk="$d"
    d=`$kdeconfig --path apps | sed -e 's/^.*://g'`
    test -d "$d" && dir_applnk="$d"
fi

# Arguments
for arg
do
  case "$arg" in
      --destdir=*)
	  DESTDIR=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --srcdir=*)
	  srcdir=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --datadir=*)
	  datadir=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --applications=*)
	  applications=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --icons=*)
	  icons=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --pixmaps=*)
	  pixmaps=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --mime=*)
	  mime=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --application=*)
	  application=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --applnk=*)
	  applnk=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --mimelnk=*)
	  mimelnk=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --menutype=*)
	  menutype=`echo "$arg" | sed -e 's/^[^=]*=//'` ;;
      --onlyexistingdirs)
	  onlyexistingdirs=yes ;;
      --dryrun|-n)
	  dryrun=yes ;;
      --help)
cat >&2 <<EOF
Usage: installer.sh [..options..]
Valid options are:
    --destdir=DIR (default: /)
    --srcdir=DIR (default: .)
    --datadir=DIR (default: $datadir)
    --applications=DIR (default: $applications)
    --icons=DIR (default: $icons)
    --pixmaps=DIR (default: $pixmaps)
    --mime_info=DIR (default: $mime_info)
    --application_registry=DIR (default: $application_registry)
    --applnk=DIR (default: $applnk)
    --mimelnk=DIR (default: $mimelnk)
    --menutype=(applnk|redhat|xdg)
    --onlyexistingdirs
EOF
          exit 0 
	  ;;
      *)
	  echo 1>&2 "$0: Unrecognized argument '$arg'."
	  echo 1>&2 "Type '$0 --help' for more information."
	  exit 10
	  ;;
  esac
done

# Utilities

run()
{
    echo "+ $*"
    test "$dryrun" = "yes" || "$@"
}

makedir()
{
    if [ ! -d $1 -a $onlyexistingdirs = no ]
    then
	makedir `dirname $1`
	run mkdir $1
    fi
}

install()
{
    if [ -d `dirname $2` -a ! -d "$2" ]
    then
	test -f $2 && run rm -f $2
	run cp $srcdir/$1 $2
	run chmod 644 $2
    fi
}

# Fixup

case "$menutype" in
    redhat) 
	test -x $desktop_file_install || menutype=xdg ;;
    applnk) 
	;;
    xdg)    
	;;
    *)
	echo 1>&2 "$0: incorrect menutype (one of applnk,redhat,xdg)"
	exit 10
	;;
esac

# Go

if [ "$icons" != no ]
then
  makedir $DESTDIR$icons/hicolor/48x48/mimetypes
  makedir $DESTDIR$icons/hicolor/32x32/mimetypes
  makedir $DESTDIR$icons/hicolor/22x22/mimetypes
  install hi48-mimetype-djvu.png $DESTDIR$icons/hicolor/48x48/mimetypes/djvu.png
  install hi48-mimetype-djvu.png $DESTDIR$icons/hicolor/32x32/mimetypes/djvu.png
  install hi48-mimetype-djvu.png $DESTDIR$icons/hicolor/22x22/mimetypes/djvu.png
fi

if [ "$pixmaps" != no ]
then
  makedir $DESTDIR$pixmaps
  install hi48-mimetype-djvu.png $DESTDIR$pixmaps/djvu.png
fi

if [ "$mime_info" != no ]
then
  makedir $DESTDIR$mime_info
  install djvu.mime $DESTDIR$mime_info/djvu.mime
  install djvu.keys $DESTDIR$mime_info/djvu.keys
fi

if [ "$application_registry" != no ]
then
  makedir $DESTDIR$application_registry
  install djvu.applications $DESTDIR$application_registry
fi

if [ "$mimelnk" != no ]
then
  makedir $DESTDIR$mimelnk/image
  install x-djvu.desktop $DESTDIR$mimelnk/image/x-djvu.desktop
fi

case "$menutype" in
    redhat)
	if [ "$applications" != no ] 
	then
 	  makedir $DESTDIR$applications
	  run $desktop_file_install --vendor= --dir=$DESTDIR$applications djview.desktop
        fi
	;;
    xdg)
	if [ "$applications" != no ] 
	then
	  makedir $DESTDIR$applications
	  install djview.desktop $DESTDIR$applications/djview.desktop
	fi
	;;
    applnk)
	if [ "$applnk" != no ] 
	then
	  makedir $DESTDIR$applnk/Graphics
	  install djview.desktop $DESTDIR$applnk/Graphics/djview.desktop
	fi
	;;
esac

