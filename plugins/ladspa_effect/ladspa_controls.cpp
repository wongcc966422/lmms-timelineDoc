/*
 * ladspa_controls.cpp - model for LADSPA-plugin controls
 *
 * Copyright (c) 2008 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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



#include "ladspa_effect.h"


ladspaControls::ladspaControls( ladspaEffect * _eff, 
						track * _track ) :
	effectControls( _eff ),
	m_effect( _eff ),
	m_processors( _eff->getProcessorCount() ),
	m_track( _track ),
	m_noLink( FALSE ),
	m_stereoLinkModel( TRUE, this )
{
	multi_proc_t controls = m_effect->getPortControls();
	m_controlCount = controls.count();

	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		control_list_t p;

		const bool linked_control = ( m_processors > 1 && proc == 0 );
		buffer_data_t last_port = NONE;

		for( multi_proc_t::iterator it = controls.begin(); 
						it != controls.end(); it++ )
		{
			if( (*it)->proc == proc )
			{
				(*it)->control = new ladspaControl( this, *it,
							m_track,
							linked_control );

				last_port = (*it)->data_type;

				p.append( (*it)->control );

				if( linked_control )
				{
					connect( (*it)->control, 
					SIGNAL( linkChanged( Uint16, bool ) ),
						this,
					SLOT( linkPort( Uint16, bool ) ) );
				}
			}
		}

		m_controls.append( p );
	}
}




ladspaControls::~ladspaControls()
{
	for( ch_cnt_t proc = 0; proc < m_processors; proc++ )
	{
		m_controls[proc].clear();
	}
	m_controls.clear();
}




void FASTCALL ladspaControls::saveSettings( QDomDocument & _doc, 
							QDomElement & _this )
{
	if( m_processors > 1 )
	{
		_this.setAttribute( "link", m_stereoLinkModel.value() );
	}
	
	multi_proc_t controls = m_effect->getPortControls();
	_this.setAttribute( "ports", controls.count() );
	for( multi_proc_t::iterator it = controls.begin(); 
						it != controls.end(); it++ )
	{
		QString n = "port" + QString::number( (*it)->proc ) + 
					QString::number( (*it)->port_id );
		(*it)->control->saveSettings( _doc, _this, n );
	}
}




void FASTCALL ladspaControls::loadSettings( const QDomElement & _this )
{
	if( m_processors > 1 )
	{
		m_stereoLinkModel.setValue( _this.attribute( "link" ).toInt() );
	}
	
	multi_proc_t controls = m_effect->getPortControls();
	for( multi_proc_t::iterator it = controls.begin(); 
						it != controls.end(); it++ )
	{
		QString n = "port" + QString::number( (*it)->proc ) + 
					QString::number( (*it)->port_id );
		(*it)->control->loadSettings( _this, n );
	}
}




void ladspaControls::linkPort( Uint16 _port, bool _state )
{
	ladspaControl * first = m_controls[0][_port];
	if( _state )
	{
		for( ch_cnt_t proc = 1; proc < m_processors; proc++ )
		{
			first->linkControls( m_controls[proc][_port] );
		}
	}
	else
	{
		for( ch_cnt_t proc = 1; proc < m_processors; proc++ )
		{
			first->unlinkControls( m_controls[proc][_port] );
		}
		m_noLink = TRUE;
		m_stereoLinkModel.setValue( FALSE );
	}
}



void ladspaControls::updateChannelLinkState( void )
{
	if( m_stereoLinkModel.value() )
	{
		for( Uint16 port = 0; 
			port < m_controlCount / m_processors;
			port++ )
		{
			m_controls[0][port]->setLink( TRUE );
		}
	}
	else if( !m_noLink )
	{
		for( Uint16 port = 0; 
			port < m_controlCount / m_processors;
			port++ )
		{
			m_controls[0][port]->setLink( FALSE );
		}
	}
	else
	{
		m_noLink = FALSE;
	}
}


#include "ladspa_controls.moc"
