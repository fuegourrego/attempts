/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
// $Id$

#include <sys/time.h>

#include <assert.h>
#include <unistd.h>

#include "kfileitem.h"

#include <qdir.h>
#include <qfile.h>
#include <qimage.h>
#include <qpixmap.h>

#include <kglobal.h>
#include <klocale.h>
#include <kmimetype.h>
#include <krun.h>

KFileItem::KFileItem( const KUDSEntry& _entry, KURL& _url, bool _determineMimeTypeOnDemand ) :
  m_entry( _entry ),
  m_url( _url ),
  m_bIsLocalURL( _url.isLocalFile() ),
  m_fileMode( (mode_t)-1 ),
  m_permissions( (mode_t)-1 ),
  m_bLink( false ),
  m_pMimeType( 0 ),
  m_bMarked( false )
{
  // extract the mode and the filename from the KIO::UDS Entry
  KUDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ ) {
  switch ((*it).m_uds) {

    case KIO::UDS_FILE_TYPE:
      m_fileMode = (mode_t)((*it).m_long);
      break;

    case KIO::UDS_ACCESS:
      m_permissions = (mode_t)((*it).m_long);
      break;

    case KIO::UDS_USER:
      m_user = ((*it).m_str);
      break;

    case KIO::UDS_GROUP:
      m_group = ((*it).m_str);
      break;

    case KIO::UDS_NAME:
      m_strText = decodeFileName( (*it).m_str );
      break;

    case KIO::UDS_URL:
      m_url = KURL((*it).m_str);
      break;

    case KIO::UDS_MIME_TYPE:
      m_pMimeType = KMimeType::mimeType((*it).m_str);
      break;

    case KIO::UDS_LINK_DEST:
      m_bLink = !(*it).m_str.isEmpty(); // we don't store the link dest
      break;
  }
  }
  init( _determineMimeTypeOnDemand );
}

KFileItem::KFileItem( mode_t _mode, const KURL& _url, bool _determineMimeTypeOnDemand ) :
  m_entry(), // warning !
  m_url( _url ),
  m_bIsLocalURL( _url.isLocalFile() ),
  m_strText( decodeFileName( _url.filename() ) ),
  m_fileMode ( _mode ), // temporary
  m_permissions( (mode_t) -1 ),
  m_bLink( false ),
  m_bMarked( false )
{
  init( _determineMimeTypeOnDemand );
}

void KFileItem::init( bool _determineMimeTypeOnDemand )
{
  // determine mode and/or permissions if unknown
  if ( m_fileMode == (mode_t) -1 || m_permissions == (mode_t) -1 )
  {
    mode_t mode = 0;
    if ( m_url.isLocalFile() )
    {
      /* directories may not have a slash at the end if
       * we want to stat() them; it requires that we
       * change into it .. which may not be allowed
       * stat("/is/unaccessible")  -> rwx------
       * stat("/is/unaccessible/") -> EPERM            H.Z.
       * This is the reason for the -1
       */
      struct stat buf;
      lstat( m_url.path( -1 ), &buf );
      mode = buf.st_mode;

      if ( S_ISLNK( mode ) )
      {
        m_bLink = true;
        stat( m_url.path( -1 ), &buf );
        mode = buf.st_mode;
      }
    }
    if ( m_fileMode == (mode_t) -1 )
      m_fileMode = mode & S_IFMT; // extract file type
    if ( m_permissions == (mode_t) -1 )
      m_permissions = mode & 0x1FF; // extract permissions
  }

  // determine the mimetype
  if (!m_pMimeType && !_determineMimeTypeOnDemand )
    m_pMimeType = KMimeType::findByURL( m_url, m_fileMode, m_bIsLocalURL );

  //  assert (m_pMimeType);
}

void KFileItem::refresh()
{
  m_fileMode = (mode_t)-1;
  m_permissions = (mode_t)-1;
  init( false );
}

void KFileItem::refreshMimeType()
{
  m_pMimeType = 0L;
  init( false ); // Will determine the mimetype
}

QPixmap KFileItem::pixmap( KIconLoader::Size _size, bool bImagePreviewAllowed ) const
{
  if ( !m_pMimeType )
  {
    if ( S_ISDIR( m_fileMode ) )
     return KGlobal::iconLoader()->loadIcon( "folder", _size );

    return KGlobal::iconLoader()->loadIcon( "mimetypes/unknown", _size );
  }

  if ( m_pMimeType->name().left(6) == "image/" && m_bIsLocalURL && bImagePreviewAllowed )
  {
    QString xvpicPath = m_url.directory() +
                  // Append .xvpics if not already in an .xvpics dir
                  ((KURL(m_url.directory()).filename(true) != ".xvpics") ? "/.xvpics/" : "/");
    xvpicPath += m_url.filename();
    QPixmap pix;

    // Is the xv pic available ?
    struct stat buff;
    bool bAvail = false;
    if ( stat( xvpicPath.local8Bit(), &buff ) == 0 )
    { // Yes
      bAvail = true;
      // Get the time of the xv pic
      time_t t1 = buff.st_mtime;
      // Get time of the orig file
      if ( lstat( m_url.path().local8Bit(), &buff ) == 0 )
      {
        time_t t2 = buff.st_mtime;
        // Is it outdated ?
        if ( t1 < t2 )
          bAvail = false;
      }
    }

    if ( bAvail )
    {
      if ( pix.load( xvpicPath ) )
        return pix;
    } else
    {
      // No xv pic, or too old -> load the orig image and create the XV pic
      if ( pix.load( m_url.path() ) )
      {
        bool bCanSave = true;
        // Create .xvpics/ if it doesn't exist
        QDir xvDir( m_url.directory() );
        if ( !xvDir.exists(".xvpics") )
        {
          bCanSave = xvDir.mkdir(".xvpics");
        }
        // Save XV file
        int w, h;
        if ( pix.width() > pix.height() )
        {
          w = QMIN( pix.width(), 80 ); // TODO make configurable for tackat :-)
          h = (int)( (float)pix.height() * ( (float)w / (float)pix.width() ) );
        }
        else
        {
          h = QMIN( pix.height(), 60 ); // TODO make configurable for tackat :-)
          w = (int)( (float)pix.width() * ( (float)h / (float)pix.height() ) );
        }
        if (bCanSave)
        {
          QImageIO iio;
          iio.setImage( pix.convertToImage().smoothScale( w, h ) );
          iio.setFileName( xvpicPath );
          iio.setFormat( "XV" );
          bCanSave = iio.write();
          // Load it
          if ( pix.load( xvpicPath ) ) return pix;
        }
        if (!bCanSave) // not "else", write may have failed !
        {
          // Ok, this is ugly and slow. Anybody knows of a better solution ?
          QImage img = pix.convertToImage().smoothScale( w, h );
          pix.convertFromImage( img );
          return pix;
        }
      }
    }
  }

  QPixmap p = m_pMimeType->pixmap( m_url, _size );
  if (p.isNull())
    warning("Pixmap not found for mimetype %s",m_pMimeType->name().latin1());
  return p;
}

bool KFileItem::acceptsDrops()
{
  // Any directory : yes
//  if ( mimetype() == "inode/directory" )
  if ( S_ISDIR( mode() ) )
    return true;

  // But only local .desktop files and executables
  if ( !m_bIsLocalURL )
    return false;

  if ( m_pMimeType && mimetype() == "application/x-desktop")
    return true;

  // Executable, shell script ... ?
  if ( access( m_url.path(), X_OK ) == 0 )
    return true;

  return false;
}

QString KFileItem::getStatusBarInfo()
{
  QString comment = determineMimeType()->comment( m_url, false );
  QString text = m_strText;
  // Extract from the KIO::UDSEntry the additional info we didn't get previously
  QString myLinkDest = linkDest();
  long mySize = size();

  QString text2 = text.copy();

  if ( m_bLink )
  {
      QString tmp;
      if ( comment.isEmpty() )
	tmp = i18n ( "Symbolic Link" );
      else
        tmp = i18n("%1 (Link)").arg(comment);
      text += "->";
      text += myLinkDest;
      text += "  ";
      text += tmp;
  }
  else if ( S_ISREG( m_fileMode ) )
  {
      if (mySize < 1024)
        text = i18n("%1 (%2 bytes)").arg(text2).arg((long) mySize));
      else
      {
	float d = (float) mySize/1024.0;
        text = i18n("%1 (%2 KB)").arg(text2).arg(KGlobal::locale()->formatNumber(d, 2));
      }
      text += "  ";
      text += comment;
  }
  else if ( S_ISDIR ( m_fileMode ) )
  {
      text += "/  ";
      text += comment;
    }
    else
    {
      text += "  ";
      text += comment;
    }	
    return text;
}

QString KFileItem::linkDest() const
{
  // Extract it from the KIO::UDSEntry
  KUDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == KIO::UDS_LINK_DEST )
      return (*it).m_str;
  // If not in the KIO::UDSEntry, or if UDSEntry empty, use readlink() [if local URL]
  if ( m_bIsLocalURL )
  {
    char buf[1000];
    int n = readlink( m_url.path( -1 ), buf, 1000 );
    if ( n != -1 )
    {
      buf[ n ] = 0;
      return QString( buf );
    }
  }
  return QString::null;
}

long KFileItem::size() const
{
  // Extract it from the KIO::UDSEntry
  KUDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == KIO::UDS_SIZE )
      return (*it).m_long;
  // If not in the KIO::UDSEntry, or if UDSEntry empty, use stat() [if local URL]
  if ( m_bIsLocalURL )
  {
    struct stat buf;
    stat( m_url.path( -1 ), &buf );
    return buf.st_size;
  }
  return 0L;
}

QString KFileItem::time( unsigned int which ) const
{
  // Extract it from the KIO::UDSEntry
  KUDSEntry::ConstIterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( (*it).m_uds == which )
      return makeTimeString( (time_t)((*it).m_long) );
  // If not in the KIO::UDSEntry, or if UDSEntry empty, use stat() [if local URL]
  if ( m_bIsLocalURL )
  {
    struct stat buf;
    stat( m_url.path( -1 ), &buf );
    time_t t = (which == KIO::UDS_MODIFICATION_TIME) ? buf.st_mtime :
               (which == KIO::UDS_ACCESS_TIME) ? buf.st_atime :
               (which == KIO::UDS_CREATION_TIME) ? buf.st_ctime : (time_t) 0;
    return makeTimeString( t );
  }
  return QString::null;
}

QString KFileItem::mimetype()
{
  return determineMimeType()->name();
}

KMimeType::Ptr KFileItem::determineMimeType()
{
  if ( !m_pMimeType )
    m_pMimeType = KMimeType::findByURL( m_url, m_fileMode, m_bIsLocalURL );

  return m_pMimeType;
}

QString KFileItem::mimeComment()
{
 KMimeType::Ptr mType = determineMimeType();
 QString comment = mType->comment( m_url, false );
  if (!comment.isEmpty())
    return comment;
  else
    return mType->name();
}

QString KFileItem::iconName()
{
  return determineMimeType()->icon(m_url, false);
}

void KFileItem::run()
{
  (void) new KRun( m_url.url(), m_fileMode, m_bIsLocalURL );
}

QString KFileItem::encodeFileName( const QString & _str )
{
  QString str( _str );

  int i = 0;
  while ( ( i = str.find( "%", i ) ) != -1 )
  {
    str.replace( i, 1, "%%");
    i += 2;
  }
  while ( ( i = str.find( "/" ) ) != -1 )
      str.replace( i, 1, "%2f");
  return str;
}

QString KFileItem::decodeFileName( const QString & _str )
{
  QString str( _str );

  int i = 0;
  while ( ( i = str.find( "%%", i ) ) != -1 )
  {
    str.replace( i, 2, "%");
    i++;
  }

  while ( ( i = str.find( "%2f" ) ) != -1 )
      str.replace( i, 3, "/");
  while ( ( i = str.find( "%2F" ) ) != -1 )
      str.replace( i, 3, "/");
  return str;
}

QString KFileItem::makeTimeString( time_t _time )
{
  QDateTime dt;
  dt.setTime_t(_time);

  return KGlobal::locale()->formatDateTime(dt);
}
