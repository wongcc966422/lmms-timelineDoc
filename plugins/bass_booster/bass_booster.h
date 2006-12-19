/*
 * bass_booster.h - bass-booster-effect-plugin
 *
 * Copyright (c) 2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
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


#ifndef _BASS_BOOSTER_H
#define _BASS_BOOSTER_H


#include "effect.h"
#include "effect_lib.h"
#include "main_window.h"
#include "bassbooster_control_dialog.h"



class bassBoosterEffect : public effect
{
public:
	bassBoosterEffect( effect::constructionData * _cdata );
	virtual ~bassBoosterEffect();
	virtual bool FASTCALL processAudioBuffer( surroundSampleFrame * _buf,
							const fpab_t _frames );
	inline virtual QString nodeName( void ) const
	{
		return( "bassboostereffect" );
	}

	virtual inline effectControlDialog * createControlDialog( track * )
	{
		return( new bassBoosterControlDialog(
					eng()->getMainWindow()->workspace(),
								this ) );
	}


private:
	effectLib::monoToStereoAdaptor<effectLib::bassBoost<> > m_bbFX;

	friend class bassBoosterControlDialog;
} ;





#endif