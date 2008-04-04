#ifndef SINGLE_SOURCE_COMPILE

/*
 * mmp.cpp - implementation of class multimediaProject
 *
 * Copyright (c) 2004-2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */


#include "mmp.h"

#include <math.h>

#include <QtCore/QFile>
#include <QtGui/QMessageBox>


#include "config_mgr.h"
#include "project_version.h"
#include "song_editor.h"


multimediaProject::typeDescStruct
		multimediaProject::s_types[multimediaProject::NumProjectTypes] =
{
	{ multimediaProject::UnknownType, "unknown" },
	{ multimediaProject::SongProject, "song" },
	{ multimediaProject::SongProjectTemplate, "songtemplate" },
#warning compat-code, use upgrade feature
	{ multimediaProject::InstrumentTrackSettings,
				"instrumenttracksettings,channelsettings" },
	{ multimediaProject::DragNDropData, "dnddata" },
	{ multimediaProject::ClipboardData, "clipboard-data" },
	{ multimediaProject::JournalData, "journaldata" },
	{ multimediaProject::EffectSettings, "effectsettings" },
	{ multimediaProject::VideoProject, "videoproject" },
	{ multimediaProject::BurnProject, "burnproject" },
	{ multimediaProject::Playlist, "playlist" }
} ;



multimediaProject::multimediaProject( ProjectTypes _project_type ) :
	QDomDocument( "multimedia-project" ),
	m_content(),
	m_head(),
	m_type( _project_type )
{
	QDomElement root = createElement( "multimedia-project" );
	root.setAttribute( "version", MMP_VERSION_STRING );
	root.setAttribute( "type", typeName( _project_type ) );
	root.setAttribute( "creator", "Linux MultiMedia Studio (LMMS)" );
	root.setAttribute( "creatorversion", VERSION );
	appendChild( root );

	m_head = createElement( "head" );
	root.appendChild( m_head );

	m_content = createElement( typeName( _project_type ) );
	root.appendChild( m_content );

}




multimediaProject::multimediaProject( const QString & _in_file_name,
							bool _is_filename,
							bool _upgrade ) :
	QDomDocument(),
	m_content(),
	m_head()
{
	QFile in_file( _in_file_name );
	if( _is_filename == TRUE )
	{
		if( !in_file.open( QIODevice::ReadOnly ) )
		{
			QMessageBox::critical( NULL,
					songEditor::tr( "Could not open file" ),
					songEditor::tr( "Could not open "
							"file %1. You probably "
							"have no rights to "
							"read this file.\n"
							"Please make sure you "
							"have at least read-"
							"access to the file "
							"and try again."
						).arg( _in_file_name ) );
			return;
		}
	}
	QString error_msg;
	int line;
	int col;
	if( _is_filename == TRUE )
	{
		bool error = FALSE;
		if( _in_file_name.section( '.', -1 ) == "mmpz" )
		{
			QString data = qUncompress( in_file.readAll() );
			error = !setContent( data, &error_msg, &line, &col );
		}
		else
		{
			error = !setContent( &in_file, &error_msg, &line,
									&col );
		}
		if( error )
		{
			QMessageBox::critical( NULL, songEditor::tr( "Error in "
							"multimedia-project" ),
					songEditor::tr( "The multimedia-"
							"project %1 seems to "
							"contain errors. LMMS "
							"will try its best "
							"to recover as much as "
							"possible data from "
							"this file."
						).arg( _in_file_name ) );
			return;
		}
		in_file.close();
	}
	else
	{
		if( !setContent( _in_file_name, &error_msg, &line, &col ) )
		{
			printf( "multimediaProject: error parsing XML-data "
					"directly given to constructor!\n" );
			return;
		}
	}

	QDomElement root = documentElement();
	m_type = type( root.attribute( "type" ) );
	QDomNode node = root.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.nodeName() == "head" )
			{
				m_head = node.toElement();
			}
			else if( node.nodeName() == typeName( m_type ) ||
#warning compat-code, use upgrade feature
					node.nodeName() == "channelsettings" )
			{
				m_content = node.toElement();
			}
		}
		node = node.nextSibling();
	}

	if( _upgrade && root.hasAttribute( "creatorversion" )
			&& root.attribute( "creatorversion" ) != VERSION )
	{
		upgrade();
	}
}




multimediaProject::~multimediaProject()
{
}




QString multimediaProject::nameWithExtension( const QString & _fn ) const
{
	switch( type() )
	{
		case SongProject:
			if( _fn.section( '.', -1 ) != "mmp" &&
					_fn.section( '.', -1 ) != "mpt" &&
					_fn.section( '.', -1 ) != "mmpz" )
			{
				if( configManager::inst()->value( "app",
						"nommpz" ).toInt() == 0 )
				{
					return( _fn + ".mmpz" );
				}
				return( _fn + ".mmp" );
			}
			break;
		case SongProjectTemplate:
			if( _fn.section( '.',-1 ) != "mpt" )
			{
				return( _fn + ".mpt" );
			}
			break;
		case InstrumentTrackSettings:
			if( _fn.section( '.', -2, -1 ) != "cs.xml" )
			{
				return( _fn + ".cs.xml" );
			}
			break;
		default: ;
	}
	return( _fn );
}




bool multimediaProject::writeFile( const QString & _fn )
{
	if( type() == SongProject || type() == SongProjectTemplate
					|| type() == InstrumentTrackSettings )
	{
		cleanMetaNodes( documentElement() );
	}


	QString fn = nameWithExtension( _fn );
	QFile outfile( fn );
	if( !outfile.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
	{
		QMessageBox::critical( NULL, songEditor::tr( "Could not write "
								"file" ),
					songEditor::tr( "Could not write file "
							"%1. You probably are "
							"not permitted to "
							"write to this file.\n"
							"Please make sure you "
							"have write-access to "
							"the file and try "
							"again."
						).arg( fn ) );
		return( FALSE );
	}
	QString xml = "<?xml version=\"1.0\"?>\n" + toString( 1 );
	if( fn.section( '.', -1 ) == "mmpz" )
	{
		outfile.write( qCompress( xml.toAscii() ) );
	}
	else
	{
		outfile.write( xml.toUtf8().constData(), xml.length() );
	}
	outfile.close();

	return( TRUE );
}




multimediaProject::ProjectTypes multimediaProject::typeOfFile(
							const QString & _fn )
{
	multimediaProject m( _fn, TRUE, FALSE );
	return( m.type() );
}




multimediaProject::ProjectTypes multimediaProject::type(
						const QString & _type_name )
{
	for( int i = 0; i < NumProjectTypes; ++i )
	{
		if( s_types[i].m_name == _type_name || (
			s_types[i].m_name.contains( "," ) && (
			s_types[i].m_name.section( ',', 0, 0 ) == _type_name ||
			s_types[i].m_name.section( ',', 1, 1 ) == _type_name ) )
							)

		{
			return( static_cast<multimediaProject::ProjectTypes>(
									i ) );
		}
	}
	return( UnknownType );
}




QString multimediaProject::typeName( ProjectTypes _project_type )
{
	if( _project_type >= UnknownType && _project_type < NumProjectTypes )
	{
		return( s_types[_project_type].m_name
#warning compat-code, use upgrade feature
				.section( ',', 0, 0 )
				);
	}
	return( s_types[UnknownType].m_name );
}




void multimediaProject::cleanMetaNodes( QDomElement _de )
{
	QDomNode node = _de.firstChild();
	while( !node.isNull() )
	{
		if( node.isElement() )
		{
			if( node.toElement().attribute( "metadata" ).toInt() )
			{
				QDomNode ns = node.nextSibling();
				_de.removeChild( node );
				node = ns;
				continue;
			}
			if( node.hasChildNodes() )
			{
				cleanMetaNodes( node.toElement() );
			}
		}
		node = node.nextSibling();
	}
}




void multimediaProject::upgrade( void )
{
	projectVersion version = documentElement().attribute(
							"creatorversion" );

	if( version < "0.2.1-svn20070501" )
	{
		QDomNodeList list = elementsByTagName( "arpandchords" );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			if( el.hasAttribute( "arpdir" ) )
			{
				int arpdir = el.attribute( "arpdir" ).toInt();
				if( arpdir > 0 )
				{
					el.setAttribute( "arpdir", arpdir - 1 );
				}
				else
				{
					el.setAttribute( "arpdisabled", "1" );
				}
			}
		}

		list = elementsByTagName( "sampletrack" );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			if( el.attribute( "vol" ) != "" )
			{
				el.setAttribute( "vol", el.attribute(
						"vol" ).toFloat() * 100.0f );
			}
			else
			{
				QDomNode node = el.namedItem(
							"automation-pattern" );
				if( !node.isElement() ||
					!node.namedItem( "vol" ).isElement() )
				{
					el.setAttribute( "vol", 100.0f );
				}
			}
		}

		list = elementsByTagName( "ladspacontrols" );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			QDomNode anode = el.namedItem( "automation-pattern" );
			QDomNode node = anode.firstChild();
			while( !node.isNull() )
			{
				if( node.isElement() )
				{
					QString name = node.nodeName();
					if( name.endsWith( "link" ) )
					{
						el.setAttribute( name,
							node.namedItem( "time" )
							.toElement()
							.attribute( "value" ) );
						QDomNode oldNode = node;
						node = node.nextSibling();
						anode.removeChild( oldNode );
						continue;
					}
				}
				node = node.nextSibling();
			}
		}

		QDomNode node = m_head.firstChild();
		while( !node.isNull() )
		{
			if( node.isElement() )
			{
				if( node.nodeName() == "bpm" )
				{
					int value = node.toElement().attribute(
							"value" ).toInt();
					if( value > 0 )
					{
						m_head.setAttribute( "bpm",
									value );
						QDomNode oldNode = node;
						node = node.nextSibling();
						m_head.removeChild( oldNode );
						continue;
					}
				}
				else if( node.nodeName() == "mastervol" )
				{
					int value = node.toElement().attribute(
							"value" ).toInt();
					if( value > 0 )
					{
						m_head.setAttribute(
							"mastervol", value );
						QDomNode oldNode = node;
						node = node.nextSibling();
						m_head.removeChild( oldNode );
						continue;
					}
				}
				else if( node.nodeName() == "masterpitch" )
				{
					m_head.setAttribute( "masterpitch",
						-node.toElement().attribute(
							"value" ).toInt() );
					QDomNode oldNode = node;
					node = node.nextSibling();
					m_head.removeChild( oldNode );
					continue;
				}
			}
			node = node.nextSibling();
		}
	}

	if( version < "0.2.1-svn20070508" )
	{
		QDomNodeList list = elementsByTagName( "arpandchords" );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			if( el.hasAttribute( "chorddisabled" ) )
			{
				el.setAttribute( "chord-enabled",
					!el.attribute( "chorddisabled" )
								.toInt() );
				el.setAttribute( "arp-enabled",
					!el.attribute( "arpdisabled" )
								.toInt() );
			}
			else if( !el.hasAttribute( "chord-enabled" ) )
			{
				el.setAttribute( "chord-enabled", TRUE );
				el.setAttribute( "arp-enabled",
					el.attribute( "arpdir" ).toInt() != 0 );
			}
		}

		list = elementsByTagName( "channeltrack" );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			el.setTagName( "instrumenttrack" );
		}

		list = elementsByTagName( "instrumenttrack" );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			if( el.hasAttribute( "vol" ) )
			{
				float value = el.attribute( "vol" ).toFloat();
				value = roundf( value * 0.585786438f );
				el.setAttribute( "vol", value );
			}
			else
			{
				QDomNodeList vol_list = el.namedItem(
							"automation-pattern" )
						.namedItem( "vol" ).toElement()
						.elementsByTagName( "time" );
				for( int j = 0; !vol_list.item( j ).isNull();
									++j )
				{
					QDomElement timeEl = list.item( j )
								.toElement();
					int value = timeEl.attribute( "value" )
								.toInt();
					value = (int)roundf( value *
								0.585786438f );
					timeEl.setAttribute( "value", value );
				}
			}
		}
	}


	if( version < "0.3.0-rc2" )
	{
		QDomNodeList list = elementsByTagName( "arpandchords" );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			if( el.attribute( "arpdir" ).toInt() > 0 )
			{
				el.setAttribute( "arpdir",
					el.attribute( "arpdir" ).toInt() - 1 );
			}
		}
	}

	if( version < "0.3.0" )
	{
		QDomNodeList list;
		while( !( list = elementsByTagName(
					"pluckedstringsynth" ) ).isEmpty() )
		{
			QDomElement el = list.item( 0 ).toElement();
			el.setTagName( "vibedstrings" );
			el.setAttribute( "active0", 1 );
		}

		while( !( list = elementsByTagName( "lb303" ) ).isEmpty() )
		{
			QDomElement el = list.item( 0 ).toElement();
			el.setTagName( "lb302" );
		}
	}

	if( version < "0.4.0-svn20080104" )
	{
		QDomNodeList list = elementsByTagName( "fx" );
		for( int i = 0; !list.item( i ).isNull(); ++i )
		{
			QDomElement el = list.item( i ).toElement();
			if( el.hasAttribute( "fxdisabled" ) &&
				el.attribute( "fxdisabled" ).toInt() == 0 )
			{
				el.setAttribute( "enabled", 1 );
			}
		}
	}
	if( version < "0.4.0-svn20080118" )
	{
		QDomNodeList list;
		while( !( list = elementsByTagName( "fx" ) ).isEmpty() )
		{
			QDomElement fxchain = list.item( 0 ).toElement();
			fxchain.setTagName( "fxchain" );
			QDomNode rack = list.item( 0 ).firstChild();
			QDomNodeList effects = rack.childNodes();
			// move items one level up
			while( effects.count() )
			{
				fxchain.appendChild( effects.at( 0 ) );
			}
			fxchain.setAttribute( "numofeffects",
				rack.toElement().attribute( "numofeffects" ) );
			fxchain.removeChild( rack );
		}
	}

	if( version < "0.4.0-svn20080129" )
	{
		QDomNodeList list;
		while( !( list =
			elementsByTagName( "arpandchords" ) ).isEmpty() )
		{
			QDomElement aac = list.item( 0 ).toElement();
			aac.setTagName( "arpeggiator" );
			QDomNode cloned = aac.cloneNode();
			cloned.toElement().setTagName( "chordcreator" );
			aac.parentNode().appendChild( cloned );
		}
	}

	if( !m_head.hasAttribute( "mastervol" ) )
	{
		m_head.setAttribute( "mastervol", 100 );
	}
//printf("%s\n", toString( 2 ).toAscii().constData());
}




#endif