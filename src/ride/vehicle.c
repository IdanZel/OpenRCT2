/*****************************************************************************
 * Copyright (c) 2014 Ted John
 * OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
 *
 * This file is part of OpenRCT2.
 *
 * OpenRCT2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include "../addresses.h"
#include "../game.h"
#include "../audio/audio.h"
#include "../audio/mixer.h"
#include "../config.h"
#include "../hook.h"
#include "../interface/viewport.h"
#include "../openrct2.h"
#include "../scenario.h"
#include "../world/map_animation.h"
#include "../world/sprite.h"
#include "../world/scenery.h"
#include "track.h"
#include "ride.h"
#include "ride_data.h"
#include "track.h"
#include "track_data.h"
#include "vehicle.h"

static void vehicle_update(rct_vehicle *vehicle);

static void vehicle_update_cable_lift_moving_to_end_of_station(rct_vehicle *vehicle);
static void vehicle_update_cable_lift_waiting_to_depart(rct_vehicle *vehicle);
static void vehicle_update_cable_lift_departing(rct_vehicle *vehicle);
static void vehicle_update_cable_lift_travelling(rct_vehicle *vehicle);
static void vehicle_update_cable_lift_arriving(rct_vehicle *vehicle);

static void vehicle_update_showing_film(rct_vehicle *vehicle);
static void vehicle_update_doing_circus_show(rct_vehicle *vehicle);
static void vehicle_update_moving_to_end_of_station(rct_vehicle *vehicle);
static void vehicle_update_waiting_for_passengers(rct_vehicle* vehicle);
static void vehicle_update_waiting_to_depart(rct_vehicle* vehicle);
static void vehicle_update_departing(rct_vehicle* vehicle);
static void vehicle_finish_departing(rct_vehicle* vehicle);
static void vehicle_update_travelling(rct_vehicle* vehicle);
static void vehicle_update_ferris_wheel_rotating(rct_vehicle* vehicle);
static void vehicle_update_rotating(rct_vehicle* vehicle);
static void vehicle_update_space_rings_operating(rct_vehicle* vehicle);
static void vehicle_update_haunted_house_operating(rct_vehicle* vehicle);
static void vehicle_update_crooked_house_operating(rct_vehicle* vehicle);
static void vehicle_update_bumpcar_mode(rct_vehicle* vehicle);
static void vehicle_update_swinging(rct_vehicle* vehicle);
static void vehicle_update_simulator_operating(rct_vehicle* vehicle);
static void vehicle_update_top_spin_operating(rct_vehicle* vehicle);
static void vehicle_update_waiting_for_cable_lift(rct_vehicle *vehicle);
static void vehicle_update_crash(rct_vehicle *vehicle);
static void vehicle_update_travelling_cable_lift(rct_vehicle* vehicle);
static void vehicle_update_travelling_boat(rct_vehicle* vehicle);
static void vehicle_update_arriving(rct_vehicle* vehicle);
static void vehicle_update_unloading_passengers(rct_vehicle* vehicle);
static void vehicle_update_crash_setup(rct_vehicle* vehicle);
static void vehicle_update_collision_setup(rct_vehicle* vehicle);
static int vehicle_update_motion_bumper_car(rct_vehicle* vehicle);
static bool vehicle_update_bumper_car_collision(rct_vehicle *vehicle, sint16 x, sint16 y, uint16 *spriteId);
static void vehicle_update_sound(rct_vehicle *vehicle);
static int vehicle_update_scream_sound(rct_vehicle *vehicle);

static void vehicle_kill_all_passengers(rct_vehicle* vehicle);

#define NO_SCREAM 254

// Size: 0x09
typedef struct {
	uint16 x;			// 0x00
	uint16 y;			// 0x02
	uint16 z;			// 0x04
	uint8 direction;	// 0x06
	uint8 var_07;
	uint8 var_08;
} rct_vehicle_info;

rct_xyz16 *unk_F64E20 = (rct_xyz16*)0x00F64E20;

const uint8 byte_9A3A14[] = { SOUND_SCREAM_8, SOUND_SCREAM_1 };
const uint8 byte_9A3A16[] = { SOUND_SCREAM_1, SOUND_SCREAM_6 };
const uint8 byte_9A3A18[] = {
	SOUND_SCREAM_3, SOUND_SCREAM_1, SOUND_SCREAM_5, SOUND_SCREAM_6,
	SOUND_SCREAM_7, SOUND_SCREAM_2, SOUND_SCREAM_4
};

static const rct_vehicle_info *vehicle_get_move_info(int cd, int typeAndDirection, int offset)
{
	const rct_vehicle_info **infoListList = RCT2_ADDRESS(0x008B8F30, rct_vehicle_info**)[cd];
	const rct_vehicle_info *infoList = infoListList[typeAndDirection];
	return &infoList[offset];
}

const uint8 DoorOpenSoundIds[] = {
	SOUND_WATER_2,
	SOUND_DOOR_OPEN
};

const uint8 DoorCloseSoundIds[] = {
	SOUND_62,
	SOUND_DOOR_CLOSE,
	SOUND_62
};

/**
*
*  rct2: 0x006BB9FF
*/
void vehicle_update_sound_params(rct_vehicle* vehicle)
{
	if (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR) && (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_TRACK_DESIGNER) || RCT2_GLOBAL(0x0141F570, uint8) == 6)) {
		if (vehicle->sound1_id != (uint8)-1 || vehicle->sound2_id != (uint8)-1) {
			if (vehicle->sprite_left != (sint16)0x8000) {
				RCT2_GLOBAL(0x009AF5A0, sint16) = vehicle->sprite_left;
				RCT2_GLOBAL(0x009AF5A2, sint16) = vehicle->sprite_top;
				RCT2_GLOBAL(0x009AF5A4, sint16) = vehicle->sprite_right;
				RCT2_GLOBAL(0x009AF5A6, sint16) = vehicle->sprite_bottom;
				sint16 v4 = RCT2_GLOBAL(0x00F438A4, rct_viewport*)->view_x;
				sint16 v5 = RCT2_GLOBAL(0x00F438A4, rct_viewport*)->view_y;
				sint16 v6 = RCT2_GLOBAL(0x00F438A4, rct_viewport*)->view_width / 4;
				sint16 v7 = RCT2_GLOBAL(0x00F438A4, rct_viewport*)->view_height / 4;
				if (!RCT2_GLOBAL(0x00F438A8, rct_window*)->classification) {
					v4 -= v6;
					v5 -= v7;
				}
				if (v4 < RCT2_GLOBAL(0x009AF5A4, sint16) && v5 < RCT2_GLOBAL(0x009AF5A6, sint16)) {
					sint16 t8 = RCT2_GLOBAL(0x00F438A4, rct_viewport*)->view_width + v4;
					sint16 t9 = RCT2_GLOBAL(0x00F438A4, rct_viewport*)->view_height + v5;
					if (!RCT2_GLOBAL(0x00F438A8, rct_window*)->classification) {
						t8 += v6 + v6;
						t9 += v7 + v7;
					}
					if (t8 >= RCT2_GLOBAL(0x009AF5A0, sint16) && t9 >= RCT2_GLOBAL(0x009AF5A2, sint16)) {
						uint16 v9 = sub_6BC2F3(vehicle);
						rct_vehicle_sound_params* i;
						for (i = &gVehicleSoundParamsList[0]; i < gVehicleSoundParamsListEnd && v9 <= i->var_A; i++);
						if (i < &gVehicleSoundParamsList[countof(gVehicleSoundParamsList)]) {
							if (gVehicleSoundParamsListEnd < &gVehicleSoundParamsList[countof(gVehicleSoundParamsList)]) {
								gVehicleSoundParamsListEnd++;
							}
							rct_vehicle_sound_params* j = gVehicleSoundParamsListEnd - 1;
							while (j >= i) {
								j--;
								*(j + 1) = *j;
							}
							i->var_A = v9;
							int pan_x = (RCT2_GLOBAL(0x009AF5A0, sint16) / 2) + (RCT2_GLOBAL(0x009AF5A4, sint16) / 2) - RCT2_GLOBAL(0x00F438A4, rct_viewport*)->view_x;
							pan_x >>= RCT2_GLOBAL(0x00F438A4, rct_viewport*)->zoom;
							pan_x += RCT2_GLOBAL(0x00F438A4, rct_viewport*)->x;

							uint16 screenwidth = RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_WIDTH, uint16);
							if (screenwidth < 64) {
								screenwidth = 64;
							}
							i->pan_x = ((((pan_x << 16) / screenwidth) - 0x8000) >> 4);

							int pan_y = (RCT2_GLOBAL(0x009AF5A2, sint16) / 2) + (RCT2_GLOBAL(0x009AF5A6, sint16) / 2) - RCT2_GLOBAL(0x00F438A4, rct_viewport*)->view_y;
							pan_y >>= RCT2_GLOBAL(0x00F438A4, rct_viewport*)->zoom;
							pan_y += RCT2_GLOBAL(0x00F438A4, rct_viewport*)->y;

							uint16 screenheight = RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_HEIGHT, uint16);
							if (screenheight < 64) {
								screenheight = 64;
							}
							i->pan_y = ((((pan_y << 16) / screenheight) - 0x8000) >> 4);

							sint32 v19 = vehicle->velocity;

							rct_ride_type* ride_type = GET_RIDE_ENTRY(vehicle->ride_subtype);
							uint8 test = ride_type->vehicles[vehicle->vehicle_type].var_5A;

							if (test & 1) {
								v19 *= 2;
							}
							if (v19 < 0) {
								v19 = -v19;
							}
							v19 >>= 5;
							v19 *= 5512;
							v19 >>= 14;
							v19 += 11025;
							v19 += 16 * vehicle->var_BF;
							i->frequency = (uint16)v19;
							i->id = vehicle->sprite_index;
							i->volume = 0;
							if (vehicle->x != (sint16)0x8000) {
								int tile_idx = (((vehicle->y & 0xFFE0) * 256) + (vehicle->x & 0xFFE0)) / 32;
								rct_map_element* map_element;
								for (map_element = RCT2_ADDRESS(RCT2_ADDRESS_TILE_MAP_ELEMENT_POINTERS, rct_map_element*)[tile_idx]; map_element->type & MAP_ELEMENT_TYPE_MASK; map_element++);
								if (map_element->base_height * 8 > vehicle->z) { // vehicle underground
									i->volume = 0x30;
								}
							}
						}
					}
				}
			}
		}
	}
}

/**
*
*  rct2: 0x006BC2F3
*/
int sub_6BC2F3(rct_vehicle* vehicle)
{
	int result = 0;
	rct_vehicle* vehicle_temp = vehicle;
	do {
		result += vehicle_temp->friction;
	} while (vehicle_temp->next_vehicle_on_train != (uint16)-1 && (vehicle_temp = GET_VEHICLE(vehicle_temp->next_vehicle_on_train)));
	sint32 v4 = vehicle->velocity;
	if (v4 < 0) {
		v4 = -v4;
	}
	result += ((uint16)v4) >> 13;
	rct_vehicle_sound* vehicle_sound = &gVehicleSoundList[0];

	while (vehicle_sound->id != vehicle->sprite_index) {
		vehicle_sound++;

		if (vehicle_sound >= &gVehicleSoundList[countof(gVehicleSoundList)]) {
			return result;
		}
	}
	return result + 300;
}

/**
*
*  rct2: 0x006BBC6B
*/
void vehicle_sounds_update()
{
	if (RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_SOUND_DEVICE, uint32) != -1 && !gGameSoundsOff && gConfigSound.sound && !gOpenRCT2Headless) {
		RCT2_GLOBAL(0x00F438A4, rct_viewport*) = (rct_viewport*)-1;
		rct_viewport* viewport = (rct_viewport*)-1;
		rct_window* window = RCT2_GLOBAL(RCT2_ADDRESS_NEW_WINDOW_PTR, rct_window*);
		while (1) {
			window--;
			if (window < RCT2_ADDRESS(RCT2_ADDRESS_WINDOW_LIST, rct_window)) {
				break;
			}
			viewport = window->viewport;
			if (viewport && viewport->flags & VIEWPORT_FLAG_SOUND_ON) {
				break;
			}
		}
		RCT2_GLOBAL(0x00F438A4, rct_viewport*) = viewport;
		if (viewport != (rct_viewport*)-1) {
			if (window) {
				RCT2_GLOBAL(0x00F438A8, rct_window*) = window;
				RCT2_GLOBAL(RCT2_ADDRESS_VOLUME_ADJUST_ZOOM, uint8) = 0;
				if (viewport->zoom) {
					RCT2_GLOBAL(RCT2_ADDRESS_VOLUME_ADJUST_ZOOM, uint8) = 35;
					if (viewport->zoom != 1) {
						RCT2_GLOBAL(RCT2_ADDRESS_VOLUME_ADJUST_ZOOM, uint8) = 70;
					}
				}
			}
			gVehicleSoundParamsListEnd = &gVehicleSoundParamsList[0];
			for (uint16 i = RCT2_GLOBAL(RCT2_ADDRESS_SPRITES_START_VEHICLE, uint16); i != SPRITE_INDEX_NULL; i = g_sprite_list[i].vehicle.next) {
				vehicle_update_sound_params(&g_sprite_list[i].vehicle);
			}
			for(int i = 0; i < countof(gVehicleSoundList); i++){
				rct_vehicle_sound* vehicle_sound = &gVehicleSoundList[i];
				if (vehicle_sound->id != (uint16)-1) {
					for (rct_vehicle_sound_params* vehicle_sound_params = &gVehicleSoundParamsList[0]; vehicle_sound_params != gVehicleSoundParamsListEnd; vehicle_sound_params++) {
						if (vehicle_sound->id == vehicle_sound_params->id) {
							goto label26;
						}
					}
					if (vehicle_sound->sound1_id != (uint16)-1) {
						Mixer_Stop_Channel(vehicle_sound->sound1_channel);
					}
					if (vehicle_sound->sound2_id != (uint16)-1) {
						Mixer_Stop_Channel(vehicle_sound->sound2_channel);
					}
					vehicle_sound->id = (uint16)-1;
				}
			label26:
				;
			}

			for (rct_vehicle_sound_params* vehicle_sound_params = &gVehicleSoundParamsList[0]; ; vehicle_sound_params++) {
			label28:
				if (vehicle_sound_params >= gVehicleSoundParamsListEnd) {
					return;
				}
				uint8 vol1 = 0xFF;
				uint8 vol2 = 0xFF;
				sint16 pan_y = vehicle_sound_params->pan_y;
				if (pan_y < 0) {
					pan_y = -pan_y;
				}
				if (pan_y > 0xFFF) {
					pan_y = 0xFFF;
				}
				pan_y -= 0x800;
				if (pan_y > 0) {
					pan_y -= 0x400;
					pan_y = -pan_y;
					pan_y = pan_y / 4;
					vol1 = LOBYTE(pan_y);
					if ((sint8)HIBYTE(pan_y) != 0) {
						vol1 = 0xFF;
						if ((sint8)HIBYTE(pan_y) < 0) {
							vol1 = 0;
						}
					}
				}

				sint16 pan_x = vehicle_sound_params->pan_x;
				if (pan_x < 0) {
					pan_x = -pan_x;
				}
				if (pan_x > 0xFFF) {
					pan_x = 0xFFF;
				}
				pan_x -= 0x800;
				if (pan_x > 0) {
					pan_x -= 0x400;
					pan_x = -pan_x;
					pan_x = pan_x / 4;
					vol2 = LOBYTE(pan_x);
					if ((sint8)HIBYTE(pan_x) != 0) {
						vol2 = 0xFF;
						if ((sint8)HIBYTE(pan_x) < 0) {
							vol2 = 0;
						}
					}
				}

				if (vol1 >= vol2) {
					vol1 = vol2;
				}
				if (vol1 < RCT2_GLOBAL(RCT2_ADDRESS_VOLUME_ADJUST_ZOOM, uint8)) {
					vol1 = 0;
				} else {
					vol1 = vol1 - RCT2_GLOBAL(RCT2_ADDRESS_VOLUME_ADJUST_ZOOM, uint8);
				}

				rct_vehicle_sound* vehicle_sound = &gVehicleSoundList[0];
				while (vehicle_sound_params->id != vehicle_sound->id) {
					vehicle_sound++;
					if (vehicle_sound >= &gVehicleSoundList[countof(gVehicleSoundList)]) {
						vehicle_sound = &gVehicleSoundList[0];
						int i = 0;
						while (vehicle_sound->id != (uint16)-1) {
							vehicle_sound++;
							i++;
							if (i >= countof(gVehicleSoundList)) {
								vehicle_sound_params = (rct_vehicle_sound_params*)((int)vehicle_sound_params + 10);
								goto label28;
							}
						}
						vehicle_sound->id = vehicle_sound_params->id;
						vehicle_sound->sound1_id = (uint16)-1;
						vehicle_sound->sound2_id = (uint16)-1;
						vehicle_sound->volume = 0x30;
						break;
					}
				}

				int tempvolume = vehicle_sound->volume;
				if (tempvolume != vehicle_sound_params->volume) {
					if (tempvolume < vehicle_sound_params->volume) {
						tempvolume += 4;
					} else {
						tempvolume -= 4;
					}
				}
				vehicle_sound->volume = tempvolume;
				if (vol1 < tempvolume) {
					vol1 = 0;
				} else {
					vol1 = vol1 - tempvolume;
				}

				// do sound1 stuff, track noise
				rct_sprite* sprite = &g_sprite_list[vehicle_sound_params->id];
				int volume = sprite->vehicle.sound1_volume;
				volume *= vol1;
				volume = volume / 8;
				volume -= 0x1FFF;
				if (volume < -10000) {
					volume = -10000;
				}
				if (sprite->vehicle.sound1_id == (uint8)-1) {
					if (vehicle_sound->sound1_id != (uint16)-1) {
						vehicle_sound->sound1_id = -1;
						Mixer_Stop_Channel(vehicle_sound->sound1_channel);
					}
				} else {
					if (vehicle_sound->sound1_id == (uint16)-1) {
						goto label69;
					}
					if (sprite->vehicle.sound1_id != vehicle_sound->sound1_id) {
						Mixer_Stop_Channel(vehicle_sound->sound1_channel);
					label69:
						vehicle_sound->sound1_id = sprite->vehicle.sound1_id;
						vehicle_sound->sound1_pan = vehicle_sound_params->pan_x;
						vehicle_sound->sound1_volume = volume;
						vehicle_sound->sound1_freq = vehicle_sound_params->frequency;
						uint16 frequency = vehicle_sound_params->frequency;
						if (RCT2_ADDRESS(0x009AF51F, uint8)[2 * sprite->vehicle.sound1_id] & 2) {
							frequency = (frequency / 2) + 4000;
						}
						uint8 looping = RCT2_ADDRESS(0x009AF51E, uint8)[2 * sprite->vehicle.sound1_id];
						int pan = vehicle_sound_params->pan_x;
						vehicle_sound->sound1_channel = Mixer_Play_Effect(sprite->vehicle.sound1_id, looping ? MIXER_LOOP_INFINITE : MIXER_LOOP_NONE, DStoMixerVolume(volume), DStoMixerPan(pan), DStoMixerRate(frequency), 0);
						goto label87;
					}
					if (volume != vehicle_sound->sound1_volume) {
						vehicle_sound->sound1_volume = volume;
						Mixer_Channel_Volume(vehicle_sound->sound1_channel, DStoMixerVolume(volume));
					}
					if (vehicle_sound_params->pan_x != vehicle_sound->sound1_pan) {
						vehicle_sound->sound1_pan = vehicle_sound_params->pan_x;
						Mixer_Channel_Pan(vehicle_sound->sound1_channel, DStoMixerPan(vehicle_sound_params->pan_x));
					}
					if (!(RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_TICKS, uint32) & 3) && vehicle_sound_params->frequency != vehicle_sound->sound1_freq) {
						vehicle_sound->sound1_freq = vehicle_sound_params->frequency;
						uint16 frequency = vehicle_sound_params->frequency;
						if (RCT2_GLOBAL(0x009AF51F, uint8*)[2 * sprite->vehicle.sound1_id] & 2) {
							frequency = (frequency / 2) + 4000;
						}
						Mixer_Channel_Rate(vehicle_sound->sound1_channel, DStoMixerRate(frequency));
					}
				}
			label87: // do sound2 stuff, screams
				sprite = &g_sprite_list[vehicle_sound_params->id];
				volume = sprite->vehicle.sound2_volume;
				volume *= vol1;
				volume = (uint16)volume / 8;
				volume -= 0x1FFF;
				if (volume < -10000) {
					volume = -10000;
				}
				if (sprite->vehicle.sound2_id == (uint8)-1) {
					if (vehicle_sound->sound2_id != (uint16)-1) {
						vehicle_sound->sound2_id = -1;
						Mixer_Stop_Channel(vehicle_sound->sound2_channel);
					}
				} else {
					if (vehicle_sound->sound2_id == (uint16)-1) {
						goto label93;
					}
					if (sprite->vehicle.sound2_id != vehicle_sound->sound2_id) {
						Mixer_Stop_Channel(vehicle_sound->sound2_channel);
					label93:
						vehicle_sound->sound2_id = sprite->vehicle.sound2_id;
						vehicle_sound->sound2_pan = vehicle_sound_params->pan_x;
						vehicle_sound->sound2_volume = volume;
						vehicle_sound->sound2_freq = vehicle_sound_params->frequency;
						uint16 frequency = vehicle_sound_params->frequency;
						if (RCT2_ADDRESS(0x009AF51F, uint8)[2 * sprite->vehicle.sound2_id] & 1) {
							frequency = 12649;
						}
						frequency = (frequency * 2) - 3248;
						if (frequency > 25700) {
							frequency = 25700;
						}
						uint8 looping = RCT2_ADDRESS(0x009AF51E, uint8)[2 * sprite->vehicle.sound2_id];
						int pan = vehicle_sound_params->pan_x;
						vehicle_sound->sound2_channel = Mixer_Play_Effect(sprite->vehicle.sound2_id, looping ? MIXER_LOOP_INFINITE : MIXER_LOOP_NONE, DStoMixerVolume(volume), DStoMixerPan(pan), DStoMixerRate(frequency), 0);
						goto label114;
					}
					if (volume != vehicle_sound->sound2_volume) {
						Mixer_Channel_Volume(vehicle_sound->sound2_channel, DStoMixerVolume(volume));
						vehicle_sound->sound2_volume = volume;
					}
					if (vehicle_sound_params->pan_x != vehicle_sound->sound2_pan) {
						vehicle_sound->sound2_pan = vehicle_sound_params->pan_x;
						Mixer_Channel_Pan(vehicle_sound->sound2_channel, DStoMixerPan(vehicle_sound_params->pan_x));
					}
					if (!(RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_TICKS, uint32) & 3) && vehicle_sound_params->frequency != vehicle_sound->sound2_freq) {
						vehicle_sound->sound2_freq = vehicle_sound_params->frequency;
						if (!(RCT2_ADDRESS(0x009AF51F, uint8)[2 * sprite->vehicle.sound2_id] & 1)) {
							uint16 frequency = (vehicle_sound_params->frequency * 2) - 3248;
							if (frequency > 25700) {
								frequency = 25700;
							}
							Mixer_Channel_Rate(vehicle_sound->sound2_channel, DStoMixerRate(frequency));
						}
					}
				}
			label114:
				;
			}
		}
	}
}

/**
 *
 *  rct2: 0x006D4204
 */
void vehicle_update_all()
{
	uint16 sprite_index;
	rct_vehicle *vehicle;

	if (RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_SCENARIO_EDITOR)
		return;

	if ((RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & SCREEN_FLAGS_TRACK_DESIGNER) && RCT2_GLOBAL(0x0141F570, uint8) != 6)
		return;


	sprite_index = RCT2_GLOBAL(RCT2_ADDRESS_SPRITES_START_VEHICLE, uint16);
	while (sprite_index != SPRITE_INDEX_NULL) {
		vehicle = &(g_sprite_list[sprite_index].vehicle);
		sprite_index = vehicle->next;

		vehicle_update(vehicle);
	}
}

/* rct2: 0x006D6956
 * returns 0 when all closed
 */
static int vehicle_close_restraints(rct_vehicle* vehicle){
	rct_ride* ride = GET_RIDE(vehicle->ride);
	int ebp = 0;
	uint16 vehicle_id = vehicle->sprite_index;

	do {
		vehicle = GET_VEHICLE(vehicle_id);
		if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_BROKEN_CAR &&
			vehicle->restraints_position != 0 &&
			(
			ride->breakdown_reason_pending == BREAKDOWN_RESTRAINTS_STUCK_OPEN ||
			ride->breakdown_reason_pending == BREAKDOWN_DOORS_STUCK_OPEN)
			){

			if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN)){

				ride->lifecycle_flags |= RIDE_LIFECYCLE_BROKEN_DOWN;

				ride_breakdown_add_news_item(vehicle->ride);

				ride->window_invalidate_flags |=
					RIDE_INVALIDATE_RIDE_MAIN |
					RIDE_INVALIDATE_RIDE_LIST |
					RIDE_INVALIDATE_RIDE_MAINTENANCE;

				ride->mechanic_status = RIDE_MECHANIC_STATUS_CALLING;

				rct_vehicle* broken_vehicle = GET_VEHICLE(ride->vehicles[ride->broken_vehicle]);
				ride->inspection_station = broken_vehicle->current_station;

				ride->breakdown_reason = ride->breakdown_reason_pending;
			}
		}
		else{
			if (vehicle->restraints_position - 20 < 0){
				vehicle->restraints_position = 0;
				continue;
			}
			vehicle->restraints_position -= 20;
		}
		invalidate_sprite_2((rct_sprite*)vehicle);
		ebp++;
	} while ((vehicle_id = vehicle->next_vehicle_on_train) != 0xFFFF);

	return ebp;
}

/* rct2: 0x006D6A2C
 * returns 0 when all open
 */
static int vehicle_open_restraints(rct_vehicle* vehicle){
	int ebp = 0;
	uint16 vehicle_id = vehicle->sprite_index;

	do {
		vehicle = GET_VEHICLE(vehicle_id);

		vehicle->var_4C = 0;
		vehicle->var_4E = 0;
		vehicle->var_4A = 0;

		rct_ride* ride = GET_RIDE(vehicle->ride);
		rct_ride_type* rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);
		rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

		if (vehicleEntry->var_14 & (1 << 2)) {
			if (abs(vehicle->var_B6) <= 700 &&
				!(vehicle->var_BA & 0x30) &&
				(
					!(vehicleEntry->var_12 & (1 << 14)) ||
					!(vehicle->var_BA & 0xF8))
				){
				vehicle->var_B6 = 0;
			}
			else {
				ebp++;

				if (abs(vehicle->var_B6) < 600) {
					vehicle->var_B6 = 600;
				}
				sint16 value = vehicle->var_B6 / 256;
				vehicle->var_BA += value;
				vehicle->var_B6 -= value;

				invalidate_sprite_2((rct_sprite*)vehicle);
				continue;
			}
		}
		if (vehicleEntry->var_11 == 6 &&
			vehicle->var_C5 != 0){

			if (vehicle->var_C8 + 0x3333 < 0xFFFF){
				vehicle->var_C8 = vehicle->var_C8 + 0x3333 - 0xFFFF;
				vehicle->var_C5++;
				vehicle->var_C5 &= 7;
				invalidate_sprite_2((rct_sprite*)vehicle);
			}
			else{
				vehicle->var_C8 += 0x3333;
			}
			ebp++;
			continue;
		}

		if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_BROKEN_CAR &&
			vehicle->restraints_position != 0xFF &&
			(
			ride->breakdown_reason_pending == BREAKDOWN_RESTRAINTS_STUCK_CLOSED ||
			ride->breakdown_reason_pending == BREAKDOWN_DOORS_STUCK_CLOSED)
			){

			if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN)){

				ride->lifecycle_flags |= RIDE_LIFECYCLE_BROKEN_DOWN;

				ride_breakdown_add_news_item(vehicle->ride);

				ride->window_invalidate_flags |=
					RIDE_INVALIDATE_RIDE_MAIN |
					RIDE_INVALIDATE_RIDE_LIST |
					RIDE_INVALIDATE_RIDE_MAINTENANCE;

				ride->mechanic_status = RIDE_MECHANIC_STATUS_CALLING;

				rct_vehicle* broken_vehicle = GET_VEHICLE(ride->vehicles[ride->broken_vehicle]);
				ride->inspection_station = broken_vehicle->current_station;

				ride->breakdown_reason = ride->breakdown_reason_pending;
			}

		}
		else{
			if (vehicle->restraints_position + 20 > 0xFF){
				vehicle->restraints_position = 255;
				continue;
			}
			vehicle->restraints_position += 20;
			invalidate_sprite_2((rct_sprite*)vehicle);
			ebp++;
		}

	} while ((vehicle_id = vehicle->next_vehicle_on_train) != 0xFFFF);

	return ebp;
}

/* rct2: 0x006D6D1F */
static void vehicle_update_measurements(rct_vehicle *vehicle)
{
	rct_ride *ride;

	ride = GET_RIDE(vehicle->ride);

	if (vehicle->status == VEHICLE_STATUS_TRAVELLING_BOAT){
		ride->lifecycle_flags |= RIDE_LIFECYCLE_TESTED;
		ride->lifecycle_flags |= RIDE_LIFECYCLE_NO_RAW_STATS;
		ride->lifecycle_flags &= ~RIDE_LIFECYCLE_TEST_IN_PROGRESS;
		vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_TESTING;
		window_invalidate_by_number(WC_RIDE, vehicle->ride);
		return;
	}

	uint8 entrance = ride->var_1F6;
	if (ride->entrances[entrance] != 0xFFFF){
		uint8 test_segment = ride->current_test_segment;

		ride->var_0E1++;
		if (ride->var_0E1 >= 32)ride->var_0E1 = 0;

		sint32 velocity = abs(vehicle->velocity);
		if (velocity > ride->max_speed){
			ride->max_speed = velocity;
		}

		if (ride->var_0E1 == 0 && velocity > 0x8000){
			ride->average_speed += velocity;
			ride->time[test_segment]++;
		}

		sint32 distance = abs(((vehicle->velocity + vehicle->var_2C) >> 10) * 42);
		if (vehicle->var_CE == 0){
			ride->length[test_segment] += distance;
		}

		if (RCT2_ADDRESS(RCT2_ADDRESS_RIDE_FLAGS, uint32)[ride->type * 2] & RIDE_TYPE_FLAG_HAS_G_FORCES){
			int vertical_g, lateral_g;
			vehicle_get_g_forces(vehicle, &vertical_g, &lateral_g);

			vertical_g += ride->previous_vertical_g;
			lateral_g += ride->previous_lateral_g;
			vertical_g >>= 1;
			lateral_g >>= 1;

			ride->previous_vertical_g = vertical_g;
			ride->previous_lateral_g = lateral_g;

			if (vertical_g <= 0){
				ride->total_air_time++;
			}

			if (vertical_g > ride->max_positive_vertical_g)
				ride->max_positive_vertical_g = vertical_g;

			if (vertical_g < ride->max_negative_vertical_g)
				ride->max_negative_vertical_g = vertical_g;

			lateral_g = abs(lateral_g);

			if (lateral_g > ride->max_lateral_g)
				ride->max_lateral_g = lateral_g;
		}
	}

	uint16 map_location = (vehicle->track_x / 32) | ((vehicle->track_y / 32) << 8);
	if (vehicle->track_z / 8 != ride->var_11F || map_location != ride->var_10C){
		ride->var_11F = vehicle->track_z / 8;
		ride->var_10C = map_location;

		if (ride->entrances[ride->var_1F6] == 0xFFFF)
			return;

		uint16 track_elem_type = vehicle->track_type / 4;
		if (track_elem_type == TRACK_ELEM_POWERED_LIFT || (vehicle->update_flags & VEHICLE_UPDATE_FLAG_0)){
			if (!(ride->testing_flags & RIDE_TESTING_POWERED_LIFT)){
				ride->testing_flags |= RIDE_TESTING_POWERED_LIFT;
				if (ride->drops + 64 < 0xFF){
					ride->drops += 64;
				}
			}
		}
		else{
			ride->testing_flags &= ~RIDE_TESTING_POWERED_LIFT;
		}

		if (ride->type == RIDE_TYPE_WATER_COASTER){
			if (track_elem_type >= TRACK_ELEM_FLAT_COVERED && track_elem_type <= TRACK_ELEM_RIGHT_QUARTER_TURN_3_TILES_COVERED){
				ride->special_track_elements |= RIDE_ELEMENT_TUNNEL_SPLASH_OR_RAPIDS;
			}
		}

		switch (track_elem_type)
		{
		case TRACK_ELEM_RAPIDS:
		case TRACK_ELEM_SPINNING_TUNNEL:
			ride->special_track_elements |= RIDE_ELEMENT_TUNNEL_SPLASH_OR_RAPIDS;
			break;
		case TRACK_ELEM_WATERFALL:
		case TRACK_ELEM_LOG_FLUME_REVERSER:
			ride->special_track_elements |= RIDE_ELEMENT_REVERSER_OR_WATERFALL;
			break;
		case TRACK_ELEM_WHIRLPOOL:
			ride->special_track_elements |= RIDE_ELEMENT_WHIRLPOOL;
			break;
		case TRACK_ELEM_WATER_SPLASH:
			if (vehicle->velocity >= 0xB0000){
				ride->special_track_elements |= RIDE_ELEMENT_TUNNEL_SPLASH_OR_RAPIDS;
			}
		}

		// ax
		uint16 track_flags = RCT2_ADDRESS(0x0099423C, uint16)[track_elem_type];

		uint32 testing_flags = ride->testing_flags;
		if (testing_flags & RIDE_TESTING_TURN_LEFT &&
			track_flags & TRACK_ELEM_FLAG_TURN_LEFT){
			// 0x800 as this is masked to CURRENT_TURN_COUNT_MASK
			ride->turn_count_default += 0x800;
		}
		else if (testing_flags & RIDE_TESTING_TURN_RIGHT &&
			track_flags & TRACK_ELEM_FLAG_TURN_RIGHT){
			// 0x800 as this is masked to CURRENT_TURN_COUNT_MASK
			ride->turn_count_default += 0x800;
		}
		else if (testing_flags & RIDE_TESTING_TURN_RIGHT ||
			testing_flags & RIDE_TESTING_TURN_LEFT){

			ride->testing_flags &= ~(
				RIDE_TESTING_TURN_LEFT |
				RIDE_TESTING_TURN_RIGHT |
				RIDE_TESTING_TURN_BANKED |
				RIDE_TESTING_TURN_SLOPED);

			uint8 turn_type = 1;
			if (!(testing_flags & RIDE_TESTING_TURN_BANKED)){
				turn_type = 2;
				if (!(testing_flags & RIDE_TESTING_TURN_SLOPED)){
					turn_type = 0;
				}
			}
			switch (ride->turn_count_default >> 11){
			case 0:
				increment_turn_count_1_element(ride, turn_type);
				break;
			case 1:
				increment_turn_count_2_elements(ride, turn_type);
				break;
			case 2:
				increment_turn_count_3_elements(ride, turn_type);
				break;
			default:
				increment_turn_count_4_plus_elements(ride, turn_type);
				break;
			}
		}
		else {
			if (track_flags & TRACK_ELEM_FLAG_TURN_LEFT){
				ride->testing_flags |= RIDE_TESTING_TURN_LEFT;
				ride->turn_count_default &= ~CURRENT_TURN_COUNT_MASK;

				if (track_flags & TRACK_ELEM_FLAG_TURN_BANKED){
					ride->testing_flags |= RIDE_TESTING_TURN_BANKED;
				}
				if (track_flags & TRACK_ELEM_FLAG_TURN_SLOPED){
					ride->testing_flags |= RIDE_TESTING_TURN_SLOPED;
				}
			}

			if (track_flags & TRACK_ELEM_FLAG_TURN_RIGHT){
				ride->testing_flags |= RIDE_TESTING_TURN_RIGHT;
				ride->turn_count_default &= ~CURRENT_TURN_COUNT_MASK;

				if (track_flags & TRACK_ELEM_FLAG_TURN_BANKED){
					ride->testing_flags |= RIDE_TESTING_TURN_BANKED;
				}
				if (track_flags & TRACK_ELEM_FLAG_TURN_SLOPED){
					ride->testing_flags |= RIDE_TESTING_TURN_SLOPED;
				}
			}
		}

		if (testing_flags & RIDE_TESTING_DROP_DOWN){
			if (vehicle->velocity < 0 || !(track_flags & TRACK_ELEM_FLAG_DOWN)){
				ride->testing_flags &= ~RIDE_TESTING_DROP_DOWN;

				sint16 z = vehicle->z / 8 - ride->start_drop_height;
				if (z < 0){
					z = abs(z);
					if (z > ride->highest_drop_height){
						ride->highest_drop_height = (uint8)z;
					}
				}
			}
		}
		else if (track_flags & TRACK_ELEM_FLAG_DOWN && vehicle->velocity >= 0){
			ride->testing_flags &= ~RIDE_TESTING_DROP_UP;
			ride->testing_flags |= RIDE_TESTING_DROP_DOWN;

			uint8 drops = ride->drops & 0x3F;
			if (drops != 0x3F)
				drops++;
			ride->drops &= ~0x3F;
			ride->drops |= drops;

			ride->start_drop_height = vehicle->z / 8;
			testing_flags &= ~RIDE_TESTING_DROP_UP;
		}

		if (testing_flags & RIDE_TESTING_DROP_UP){
			if (vehicle->velocity > 0 || !(track_flags & TRACK_ELEM_FLAG_UP)){
				ride->testing_flags &= ~RIDE_TESTING_DROP_UP;

				sint16 z = vehicle->z / 8 - ride->start_drop_height;
				if (z < 0){
					z = abs(z);
					if (z > ride->highest_drop_height){
						ride->highest_drop_height = (uint8)z;
					}
				}
			}
		}
		else if (track_flags & TRACK_ELEM_FLAG_UP && vehicle->velocity <= 0){
			ride->testing_flags &= ~RIDE_TESTING_DROP_DOWN;
			ride->testing_flags |= RIDE_TESTING_DROP_UP;

			uint8 drops = ride->drops & 0x3F;
			if (drops != 0x3F)
				drops++;
			ride->drops &= ~0x3F;
			ride->drops |= drops;

			ride->start_drop_height = vehicle->z / 8;
		}

		if (track_flags & TRACK_ELEM_FLAG_INVERSION){
			uint8 inversions = ride->inversions & 0x1F;
			if (inversions != 0x1F)
				inversions++;

			ride->inversions &= ~0x1F;
			ride->inversions |= inversions;
		}

		if (track_flags & TRACK_ELEM_FLAG_HELIX){
			uint8 helixes = ride_get_helix_sections(ride);
			if (helixes != 0x1F)
				helixes++;

			ride->special_track_elements &= ~0x1F;
			ride->special_track_elements |= helixes;
		}

	}

	if (ride->entrances[ride->var_1F6] == 0xFFFF)
		return;

	sint16 x, y;
	x = vehicle->x;
	y = vehicle->y;

	if (x == SPRITE_LOCATION_NULL){
		ride->testing_flags &= ~RIDE_TESTING_SHELTERED;
		return;
	}

	rct_map_element* map_element = map_get_surface_element_at(x / 32, y / 32);
	if (map_element->base_height * 8 <= vehicle->z){

		bool cover_found = false;
		do{
			if (map_element_get_type(map_element) == MAP_ELEMENT_TYPE_SCENERY_MULTIPLE) {
				cover_found = true;
				break;
			}

			if (map_element_get_type(map_element) == MAP_ELEMENT_TYPE_PATH) {
				cover_found = true;
				break;
			}

			if (map_element_get_type(map_element) != MAP_ELEMENT_TYPE_SCENERY)
				continue;

			rct_scenery_entry* scenery = g_smallSceneryEntries[map_element->properties.scenery.type];
			if (scenery->small_scenery.flags & SMALL_SCENERY_FLAG_FULL_TILE) {
				cover_found = true;
				break;
			}
		} while (!map_element_is_last_for_tile(map_element++));

		if (cover_found == false) {
			ride->testing_flags &= ~RIDE_TESTING_SHELTERED;
			return;
		}
	}

	if (!(ride->testing_flags & RIDE_TESTING_SHELTERED)){
		ride->testing_flags |= RIDE_TESTING_SHELTERED;

		uint8 num_sheltered_sections = ride->num_sheltered_sections & 0x1F;
		if (num_sheltered_sections != 0x1F)
			num_sheltered_sections++;
		ride->num_sheltered_sections &= ~0x1F;
		ride->num_sheltered_sections |= num_sheltered_sections;

		if (vehicle->var_1F != 0){
			ride->num_sheltered_sections |= (1 << 5);
		}

		if (vehicle->var_20 != 0){
			ride->num_sheltered_sections |= (1 << 6);
		}
	}

	sint32 distance = ((vehicle->velocity + vehicle->var_2C) >> 10) * 42;
	if (distance < 0)return;

	ride->sheltered_length += distance;
}

static uint16 sub_6D7AC0(int currentSoundId, int currentVolume, int targetSoundId, int targetVolume)
{
	if (currentSoundId != 255) {
		if (currentSoundId == targetSoundId) {
			currentVolume = min(currentVolume + 15, targetVolume);
			return (currentVolume << 8) | currentSoundId;
		} else {
			currentVolume -= 9;
			if (currentVolume >= 80)
				return (currentVolume << 8) | currentSoundId;
		}
	}

	// Begin sound at quarter volume
	currentSoundId = targetSoundId;
	currentVolume = targetVolume == 255 ? 255 : targetVolume / 4;

	return (currentVolume << 8) | currentSoundId;
}

/**
 *
 *  rct2: 0x006D77F2
 */
static void vehicle_update(rct_vehicle *vehicle)
{
	rct_ride *ride;
	rct_ride_type *rideEntry;

	// The cable lift uses the ride type of NULL
	if (vehicle->ride_subtype == RIDE_TYPE_NULL) {
		switch (vehicle->status) {
		case VEHICLE_STATUS_MOVING_TO_END_OF_STATION:
			vehicle_update_cable_lift_moving_to_end_of_station(vehicle);
			break;
		case VEHICLE_STATUS_WAITING_FOR_PASSENGERS:
			// Stays in this state until a train puts it into next state
			break;
		case VEHICLE_STATUS_WAITING_TO_DEPART:
			vehicle_update_cable_lift_waiting_to_depart(vehicle);
			break;
		case VEHICLE_STATUS_DEPARTING:
			vehicle_update_cable_lift_departing(vehicle);
			break;
		case VEHICLE_STATUS_TRAVELLING:
			vehicle_update_cable_lift_travelling(vehicle);
			break;
		case VEHICLE_STATUS_ARRIVING:
			vehicle_update_cable_lift_arriving(vehicle);
			break;
		}
		return;
	}

	rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);

	rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

	ride = GET_RIDE(vehicle->ride);
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_TESTING)
		vehicle_update_measurements(vehicle);

	RCT2_GLOBAL(0x00F64E34, uint8) = 255;
	if (ride->lifecycle_flags & (RIDE_LIFECYCLE_BREAKDOWN_PENDING | RIDE_LIFECYCLE_BROKEN_DOWN)) {
		RCT2_GLOBAL(0x00F64E34, uint8) = ride->breakdown_reason_pending;
		if ((vehicleEntry->var_14 & 8) && ride->breakdown_reason_pending == BREAKDOWN_SAFETY_CUT_OUT) {
			if (!(vehicleEntry->var_14 & 0x2000) ||
				(
					vehicle->var_1F == 2 &&
					vehicle->velocity <= 0x20000
				)
			) {
				vehicle->update_flags |= VEHICLE_UPDATE_FLAG_7;
			}
		}
	}

	switch (vehicle->status) {
	case VEHICLE_STATUS_MOVING_TO_END_OF_STATION:
		vehicle_update_moving_to_end_of_station(vehicle);
		break;
	case VEHICLE_STATUS_WAITING_FOR_PASSENGERS:
		vehicle_update_waiting_for_passengers(vehicle);
		break;
	case VEHICLE_STATUS_WAITING_TO_DEPART:
		vehicle_update_waiting_to_depart(vehicle);
		break;
	case VEHICLE_STATUS_CRASHING:
	case VEHICLE_STATUS_CRASHED:
		vehicle_update_crash(vehicle);
		break;
	case VEHICLE_STATUS_TRAVELLING_BUMPER_CARS:
		vehicle_update_bumpcar_mode(vehicle);
		break;
	case VEHICLE_STATUS_SWINGING:
		vehicle_update_swinging(vehicle);
		break;
	case VEHICLE_STATUS_SIMULATOR_OPERATING:
		vehicle_update_simulator_operating(vehicle);
		break;
	case VEHICLE_STATUS_TOP_SPIN_OPERATING:
		vehicle_update_top_spin_operating(vehicle);
		break;
	case VEHICLE_STATUS_FERRIS_WHEEL_ROTATING:
		vehicle_update_ferris_wheel_rotating(vehicle);
		break;
	case VEHICLE_STATUS_SPACE_RINGS_OPERATING:
		vehicle_update_space_rings_operating(vehicle);
		break;
	case VEHICLE_STATUS_HAUNTED_HOUSE_OPERATING:
		vehicle_update_haunted_house_operating(vehicle);
		break;
	case VEHICLE_STATUS_CROOKED_HOUSE_OPERATING:
		vehicle_update_crooked_house_operating(vehicle);
		break;
	case VEHICLE_STATUS_ROTATING:
		vehicle_update_rotating(vehicle);
		break;
	case VEHICLE_STATUS_DEPARTING:
		vehicle_update_departing(vehicle);
		break;
	case VEHICLE_STATUS_TRAVELLING:
		vehicle_update_travelling(vehicle);
		break;
	case VEHICLE_STATUS_TRAVELLING_CABLE_LIFT:
		vehicle_update_travelling_cable_lift(vehicle);
		break;
	case VEHICLE_STATUS_TRAVELLING_BOAT:
		vehicle_update_travelling_boat(vehicle);
		break;
	case VEHICLE_STATUS_ARRIVING:
		vehicle_update_arriving(vehicle);
		break;
	case VEHICLE_STATUS_UNLOADING_PASSENGERS:
		vehicle_update_unloading_passengers(vehicle);
		break;
	case VEHICLE_STATUS_WAITING_FOR_CABLE_LIFT:
		vehicle_update_waiting_for_cable_lift(vehicle);
		break;
	case VEHICLE_STATUS_SHOWING_FILM:
		vehicle_update_showing_film(vehicle);
		break;
	case VEHICLE_STATUS_DOING_CIRCUS_SHOW:
		vehicle_update_doing_circus_show(vehicle);
	}

	vehicle_update_sound(vehicle);
}

/* rct2: 0x006DF8A4 */
static void vehicle_update_cable_lift_moving_to_end_of_station(rct_vehicle *vehicle)
{
	if (vehicle->velocity >= -439800)
		vehicle->var_2C = -2932;

	if (vehicle->velocity < -439800) {
		vehicle->velocity -= vehicle->velocity / 16;
		vehicle->var_2C = 0;
	}

	if (!(vehicle_update_track_motion_cable_lift(vehicle) & (1 << 0)))
		return;

	vehicle->velocity = 0;
	vehicle->var_2C = 0;
	vehicle->status = VEHICLE_STATUS_WAITING_FOR_PASSENGERS;
}

/* rct2: 0x006DF8F1 */
static void vehicle_update_cable_lift_waiting_to_depart(rct_vehicle *vehicle)
{
	if (vehicle->velocity >= -58640)
		vehicle->var_2C = -14660;

	if (vehicle->velocity < -58640) {
		vehicle->velocity -= vehicle->velocity / 16;
		vehicle->var_2C = 0;
	}

	vehicle_update_track_motion_cable_lift(vehicle);

	// Next check to see if the second part of the cable lift
	// is at the front of the passenger vehicle to simulate the
	// cable being attached underneath the train.
	rct_vehicle* passengerVehicle = GET_VEHICLE(vehicle->var_C0);
	rct_vehicle* cableLiftSecondPart = GET_VEHICLE(vehicle->prev_vehicle_on_ride);

	sint16 dist_x = abs(passengerVehicle->x - cableLiftSecondPart->x);
	sint16 dist_y = abs(passengerVehicle->y - cableLiftSecondPart->y);

	if (dist_x + dist_y > 2)
		return;

	vehicle->velocity = 0;
	vehicle->var_2C = 0;
	vehicle->status = VEHICLE_STATUS_DEPARTING;
	vehicle->sub_state = 0;
}

/* rct2: 0x006DF97A */
static void vehicle_update_cable_lift_departing(rct_vehicle *vehicle)
{
	vehicle->sub_state++;
	if (vehicle->sub_state < 16)
		return;

	rct_vehicle* passengerVehicle = GET_VEHICLE(vehicle->var_C0);
	vehicle->status = VEHICLE_STATUS_TRAVELLING;
	passengerVehicle->status = VEHICLE_STATUS_TRAVELLING_CABLE_LIFT;
}

/* rct2: 0x006DF99C */
static void vehicle_update_cable_lift_travelling(rct_vehicle *vehicle)
{
	rct_vehicle* passengerVehicle = GET_VEHICLE(vehicle->var_C0);

	vehicle->velocity = min(passengerVehicle->velocity, 439800);
	vehicle->var_2C = 0;
	if (passengerVehicle->update_flags & VEHICLE_UPDATE_FLAG_BROKEN_TRAIN)
		return;

	if (!(vehicle_update_track_motion_cable_lift(vehicle) & (1 << 1)))
		return;

	vehicle->velocity = 0;
	vehicle->var_2C = 0;
	vehicle->status = VEHICLE_STATUS_ARRIVING;
	vehicle->sub_state = 0;
}

/* rct2: 0x006DF9F0 */
static void vehicle_update_cable_lift_arriving(rct_vehicle *vehicle)
{
	vehicle->sub_state++;
	if (vehicle->sub_state >= 64)
		vehicle->status = VEHICLE_STATUS_MOVING_TO_END_OF_STATION;
}

/**
*
*  rct2: 0x006D7BCC
*/
static void vehicle_update_moving_to_end_of_station(rct_vehicle *vehicle){
	rct_ride* ride = GET_RIDE(vehicle->ride);
	int eax, ebx;

	switch (ride->mode){
	case RIDE_MODE_UPWARD_LAUNCH:
	case RIDE_MODE_ROTATING_LIFT:
	case RIDE_MODE_DOWNWARD_LAUNCH:
	case RIDE_MODE_FREEFALL_DROP:
		if (vehicle->velocity >= -131940){
			vehicle->var_2C = -3298;
		}
		if (vehicle->velocity < -131940){
			vehicle->velocity = vehicle->velocity / 16;
			vehicle->var_2C = 0;
		}

		eax = vehicle_update_track_motion(vehicle, &ebx);
		if (!(eax&(1 << 5)))
			break;
		//Fall through to next case
	case RIDE_MODE_BUMPERCAR:
	case RIDE_MODE_SWING:
	case RIDE_MODE_ROTATION:
	case RIDE_MODE_FORWARD_ROTATION:
	case RIDE_MODE_BACKWARD_ROTATION:
	case RIDE_MODE_FILM_AVENGING_AVIATORS:
	case RIDE_MODE_FILM_THRILL_RIDERS:
	case RIDE_MODE_BEGINNERS:
	case RIDE_MODE_INTENSE:
	case RIDE_MODE_BERSERK:
	case RIDE_MODE_3D_FILM_MOUSE_TAILS:
	case RIDE_MODE_3D_FILM_STORM_CHASERS:
	case RIDE_MODE_3D_FILM_SPACE_RAIDERS:
	case RIDE_MODE_SPACE_RINGS:
	case RIDE_MODE_HAUNTED_HOUSE:
	case RIDE_MODE_CROOKED_HOUSE:
	case RIDE_MODE_CIRCUS_SHOW:
		vehicle->current_station = 0;
		vehicle->velocity = 0;
		vehicle->var_2C = 0;
		vehicle->status = VEHICLE_STATUS_WAITING_FOR_PASSENGERS;
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		break;
	default:
	{
		rct_ride_type* rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);
		rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

		if (!(vehicleEntry->var_14 & (1 << 3))){
			if (vehicle->velocity <= 131940){
				vehicle->var_2C = 3298;
			}
		}
		if (vehicle->velocity > 131940){
			vehicle->velocity = vehicle->velocity / 16;
			vehicle->var_2C = 0;
		}

		int station;
		eax = vehicle_update_track_motion(vehicle, &station);

		if (eax & (1 << 1)){
			vehicle->velocity = 0;
			vehicle->var_2C = 0;
			vehicle->sub_state++;

			if (ride->mode == RIDE_MODE_RACE &&
				vehicle->sub_state >= 40){
				vehicle->status = VEHICLE_STATUS_WAITING_FOR_PASSENGERS;
				vehicle->sub_state = 0;
				vehicle_invalidate_window(vehicle);
				break;
			}
		}
		else{
			if (vehicle->velocity > 98955){
				vehicle->sub_state = 0;
			}
		}

		if (!(eax & (1 << 0)))
			break;

		vehicle->current_station = station;
		vehicle->velocity = 0;
		vehicle->var_2C = 0;
		vehicle->status = VEHICLE_STATUS_WAITING_FOR_PASSENGERS;
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		break;
	}
	}
}

/* 0x006D7FB4 */
static void train_ready_to_depart(rct_vehicle* vehicle, uint8 num_peeps_on_train, uint8 num_used_seats){

	if (num_peeps_on_train != num_used_seats)
		return;

	rct_ride* ride = GET_RIDE(vehicle->ride);

	if (ride->status == RIDE_STATUS_OPEN &&
		!(ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN) &&
		!(vehicle->update_flags & VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART)){
		return;
	}

	if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN)){
		if (ride->status != RIDE_STATUS_CLOSED ||
			ride->num_riders != 0){
			ride->train_at_station[vehicle->current_station] = 0xFF;
			vehicle->sub_state = 2;
			return;
		}
	}

	if (ride->mode == RIDE_MODE_FORWARD_ROTATION ||
		ride->mode == RIDE_MODE_BACKWARD_ROTATION){

		uint8 peep = ((-vehicle->var_1F) / 8) & 0xF;
		if (vehicle->peep[peep] != 0xFFFF){
			ride->train_at_station[vehicle->current_station] = 0xFF;
			vehicle->status = VEHICLE_STATUS_UNLOADING_PASSENGERS;
			vehicle->sub_state = 2;
			vehicle_invalidate_window(vehicle);
			return;
		}

		if (vehicle->num_peeps == 0)
			return;

		ride->train_at_station[vehicle->current_station] = 0xFF;
		vehicle->sub_state = 2;
		return;
	}

	if (num_peeps_on_train == 0)
		return;

	ride->train_at_station[vehicle->current_station] = 0xFF;
	vehicle->status = VEHICLE_STATUS_UNLOADING_PASSENGERS;
	vehicle->sub_state = 2;
	vehicle_invalidate_window(vehicle);
}

/* 0x006D7DA1 */
static void vehicle_update_waiting_for_passengers(rct_vehicle* vehicle){
	vehicle->velocity = 0;

	rct_ride* ride = GET_RIDE(vehicle->ride);

	if (vehicle->sub_state == 0){
		if (vehicle_open_restraints(vehicle))
			return;

		if (ride->entrances[vehicle->current_station] == 0xFFFF){
			ride->train_at_station[vehicle->current_station] = 0xFF;
			vehicle->sub_state = 2;
			return;
		}

		uint8 train_index = 0;
		while (ride->vehicles[train_index] != vehicle->sprite_index)train_index++;

		if (ride->train_at_station[vehicle->current_station] != 0xFF)
			return;

		ride->train_at_station[vehicle->current_station] = train_index;
		vehicle->sub_state = 1;
		vehicle->var_C0 = 0;

		invalidate_sprite_2((rct_sprite*)vehicle);
		return;
	}
	else if (vehicle->sub_state == 1){
		if (vehicle->var_C0 != 0xFFFF)
			vehicle->var_C0++;

		vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;

		// 0xF64E31, 0xF64E32, 0xF64E33
		uint8 num_peeps_on_train = 0, num_used_seats_on_train = 0, num_seats_on_train = 0;

		for (uint16 sprite_id = vehicle->sprite_index; sprite_id != 0xFFFF;){
			rct_vehicle* train_vehicle = GET_VEHICLE(sprite_id);

			num_peeps_on_train += train_vehicle->num_peeps;
			num_used_seats_on_train += train_vehicle->next_free_seat;
			num_seats_on_train += train_vehicle->num_seats;

			sprite_id = train_vehicle->next_vehicle_on_train;
		}

		num_seats_on_train &= 0x7F;

		if (!(RCT2_ADDRESS(RCT2_ADDRESS_RIDE_FLAGS, uint32)[ride->type * 2] & RIDE_TYPE_FLAG_NO_TEST_MODE)){
			if (vehicle->var_C0 < 20){
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}
		}
		else{
			if (num_peeps_on_train == 0){
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}
		}

		if (RCT2_ADDRESS(RCT2_ADDRESS_RIDE_FLAGS, uint32)[ride->type * 2] & RIDE_TYPE_FLAG_HAS_LOAD_OPTIONS){
			if (ride->depart_flags & RIDE_DEPART_WAIT_FOR_MINIMUM_LENGTH){
				if (ride->min_waiting_time * 32 > vehicle->var_C0){
					train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
					return;
				}
			}
			if (ride->depart_flags & RIDE_DEPART_WAIT_FOR_MAXIMUM_LENGTH){
				if (ride->max_waiting_time * 32 < vehicle->var_C0){
					vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;
					train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
					return;
				}
			}
		}

		if (ride->depart_flags & RIDE_DEPART_LEAVE_WHEN_ANOTHER_ARRIVES){

			for (int i = 0; i < 32; ++i){
				uint16 train_id = ride->vehicles[i];
				if (train_id == 0xFFFF)
					continue;

				if (train_id == vehicle->sprite_index)
					continue;

				rct_vehicle* train = GET_VEHICLE(train_id);

				if (train->status == VEHICLE_STATUS_UNLOADING_PASSENGERS ||
					train->status == VEHICLE_STATUS_MOVING_TO_END_OF_STATION){
					if (train->current_station == vehicle->current_station){
						vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;
						train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
						return;
					}
				}
			}
		}

		if (RCT2_ADDRESS(RCT2_ADDRESS_RIDE_FLAGS, uint32)[ride->type * 2] & RIDE_TYPE_FLAG_HAS_LOAD_OPTIONS &&
			ride->depart_flags & RIDE_DEPART_WAIT_FOR_LOAD){

			if (num_peeps_on_train == num_seats_on_train){
				vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}

			uint8 load = ride->depart_flags & RIDE_DEPART_WAIT_FOR_LOAD_MASK;
			if (load == 3){
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}

			uint8 three_quater_seats = (3 * num_seats_on_train) / 4;
			if (three_quater_seats != 0 && num_peeps_on_train >= three_quater_seats){
				vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}

			if (load == 2){
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}

			if (num_seats_on_train / 2 != 0 && num_peeps_on_train >= num_seats_on_train / 2){
				vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}

			if (load == 1){
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}

			if (num_seats_on_train / 4 != 0 && num_peeps_on_train >= num_seats_on_train / 4){
				vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}

			if (load == 0){
				vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;
				train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
				return;
			}

			if (num_peeps_on_train != 0){
				vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;
			}
			train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
			return;
		}
		vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TRAIN_READY_DEPART;
		train_ready_to_depart(vehicle, num_peeps_on_train, num_used_seats_on_train);
		return;
	}

	if (vehicle_close_restraints(vehicle))
		return;

	vehicle->velocity = 0;
	vehicle->status = VEHICLE_STATUS_WAITING_TO_DEPART;
	vehicle->sub_state = 0;
	vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_WAIT_ON_ADJACENT;

	if (ride->depart_flags & RIDE_DEPART_SYNCHRONISE_WITH_ADJACENT_STATIONS){
		vehicle->update_flags |= VEHICLE_UPDATE_FLAG_WAIT_ON_ADJACENT;
	}

	vehicle_invalidate_window(vehicle);
}

/* rct2: 0x006D91BF */
static void vehicle_update_bumpcar_mode(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);
	rct_ride_type* rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);
	rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

	if (vehicleEntry->var_12 & (1 << 7) && vehicle->var_C5 != 1) {
		vehicle->var_C5 = 1;
		invalidate_sprite_2((rct_sprite*)vehicle);
	}

	vehicle_update_motion_bumper_car(vehicle);

	// Update the length of time vehicle has been in bumper mode
	if (vehicle->sub_state++ == 0xFF) {
		vehicle->var_CE++;
	}

	if (ride->lifecycle_flags & RIDE_LIFECYCLE_PASS_STATION_NO_STOPPING)
		return;

	vehicle->var_C5 = 0;
	invalidate_sprite_2((rct_sprite*)vehicle);
	vehicle->velocity = 0;
	vehicle->var_2C = 0;
	vehicle->status = VEHICLE_STATUS_UNLOADING_PASSENGERS;
	vehicle->sub_state = 0;
	vehicle_invalidate_window(vehicle);
}

/* rct2: 0x006D808BE */
static void vehicle_update_waiting_to_depart(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);
	bool shouldBreak = false;
	if (ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN) {
		switch (ride->breakdown_reason_pending) {
		case BREAKDOWN_RESTRAINTS_STUCK_CLOSED:
		case BREAKDOWN_RESTRAINTS_STUCK_OPEN:
		case BREAKDOWN_DOORS_STUCK_CLOSED:
		case BREAKDOWN_DOORS_STUCK_OPEN:
			break;
		default:
			shouldBreak = true;
			break;
		}
	}

	bool skipCheck = false;
	if (shouldBreak == true || ride->status != RIDE_STATUS_OPEN) {
		if (ride->mode == RIDE_MODE_FORWARD_ROTATION ||
			ride->mode == RIDE_MODE_BACKWARD_ROTATION) {
			uint8 bl = ((-vehicle->var_1F) >> 3) & 0xF;
			if (vehicle->peep[bl] == 0xFFFF) {
				if (vehicle->num_peeps == 0) {
					skipCheck = true;
				}
			}
			else {
				if (ride->exits[vehicle->current_station] != 0xFFFF) {
					vehicle->status = VEHICLE_STATUS_UNLOADING_PASSENGERS;
					vehicle->sub_state = 0;
					vehicle_invalidate_window(vehicle);
					return;
				}
			}
		}
		else {
			uint16 spriteId = vehicle->sprite_index;
			for (rct_vehicle* curVehicle; spriteId != 0xFFFF; spriteId = curVehicle->next_vehicle_on_train) {
				curVehicle = GET_VEHICLE(spriteId);

				if (curVehicle->num_peeps != 0) {
					if (ride->exits[vehicle->current_station] != 0xFFFF) {
						vehicle->status = VEHICLE_STATUS_UNLOADING_PASSENGERS;
						vehicle->sub_state = 0;
						vehicle_invalidate_window(vehicle);
						return;
					}
					break;
				}
			}
		}
	}

	if (skipCheck == false) {
		if (!(ride->station_depart[vehicle->current_station] & STATION_DEPART_FLAG))
			return;
	}

	if (RCT2_ADDRESS(RCT2_ADDRESS_RIDE_FLAGS, uint32)[ride->type * 2]
		& RIDE_TYPE_FLAG_CAN_SYNCHRONISE_ADJACENT_STATIONS) {
		if (ride->depart_flags & RIDE_DEPART_SYNCHRONISE_WITH_ADJACENT_STATIONS) {
			if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_WAIT_ON_ADJACENT) {
				if (RCT2_CALLPROC_X(0x006DE287, 0, 0, 0, 0, (int)vehicle, 0, 0) & 0x100)
					return;
			}
		}
	}

	vehicle->status = VEHICLE_STATUS_DEPARTING;
	vehicle->sub_state = 0;

	if (ride->lifecycle_flags & RIDE_LIFECYCLE_CABLE_LIFT) {
		RCT2_GLOBAL(0x00F441D2, uint8) = vehicle->ride;
		rct_xy_element track;
		int z;
		int direction;

		if (track_block_get_next_from_zero(
			vehicle->track_x,
			vehicle->track_y,
			vehicle->track_z,
			vehicle->ride,
			(uint8)(vehicle->track_direction & 0x3),
			&track,
			&z,
			&direction)) {

			if (track_element_is_cable_lift(track.element)) {
				vehicle->status = VEHICLE_STATUS_WAITING_FOR_CABLE_LIFT;
			}
		}
	}

	switch (ride->mode) {
	case RIDE_MODE_BUMPERCAR:
		vehicle->status = VEHICLE_STATUS_TRAVELLING_BUMPER_CARS;
		vehicle_invalidate_window(vehicle);
		// Bumper mode uses sub_state / var_CE to tell how long
		// the vehicle has been ridden.
		vehicle->sub_state = 0;
		vehicle->var_CE = 0;
		vehicle_update_bumpcar_mode(vehicle);
		break;
	case RIDE_MODE_SWING:
		vehicle->status = VEHICLE_STATUS_SWINGING;
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		vehicle->var_CE = 0;
		vehicle->var_4C = 0xFFFF;
		vehicle_update_swinging(vehicle);
		break;
	case RIDE_MODE_ROTATION:
		vehicle->status = VEHICLE_STATUS_ROTATING;
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		vehicle->var_CE = 0;
		vehicle->var_4C = 0xFFFF;
		vehicle_update_rotating(vehicle);
		break;
	case RIDE_MODE_FILM_AVENGING_AVIATORS:
	case RIDE_MODE_FILM_THRILL_RIDERS:
		vehicle->status = VEHICLE_STATUS_SIMULATOR_OPERATING;
		vehicle->sub_state = 0;
		if (ride->mode == RIDE_MODE_FILM_THRILL_RIDERS)
			vehicle->sub_state = 1;
		vehicle_invalidate_window(vehicle);
		vehicle->var_4C = 0xFFFF;
		vehicle_update_simulator_operating(vehicle);
		break;
	case RIDE_MODE_BEGINNERS:
	case RIDE_MODE_INTENSE:
	case RIDE_MODE_BERSERK:
		vehicle->status = VEHICLE_STATUS_TOP_SPIN_OPERATING;
		vehicle_invalidate_window(vehicle);

		switch (ride->mode) {
		case RIDE_MODE_BEGINNERS:
			vehicle->sub_state = 0;
			break;
		case RIDE_MODE_INTENSE:
			vehicle->sub_state = 1;
			break;
		case RIDE_MODE_BERSERK:
			vehicle->sub_state = 2;
			break;
		}
		vehicle->var_4C = 0xFFFF;
		vehicle->var_1F = 0;
		vehicle->var_20 = 0;
		vehicle_update_top_spin_operating(vehicle);
		break;
	case RIDE_MODE_FORWARD_ROTATION:
	case RIDE_MODE_BACKWARD_ROTATION:
		vehicle->status = VEHICLE_STATUS_FERRIS_WHEEL_ROTATING;
		vehicle->sub_state = vehicle->var_1F;
		vehicle_invalidate_window(vehicle);
		vehicle->var_CE = 0;
		vehicle->var_4C = 0x808;
		vehicle_update_ferris_wheel_rotating(vehicle);
		break;
	case RIDE_MODE_3D_FILM_MOUSE_TAILS:
	case RIDE_MODE_3D_FILM_STORM_CHASERS:
	case RIDE_MODE_3D_FILM_SPACE_RAIDERS:
		vehicle->status = VEHICLE_STATUS_SHOWING_FILM;
		vehicle_invalidate_window(vehicle);
		switch (ride->mode) {
		case RIDE_MODE_3D_FILM_MOUSE_TAILS:
			vehicle->sub_state = 0;
			break;
		case RIDE_MODE_3D_FILM_STORM_CHASERS:
			vehicle->sub_state = 1;
			break;
		case RIDE_MODE_3D_FILM_SPACE_RAIDERS:
			vehicle->sub_state = 2;
			break;
		}
		vehicle->var_4C = 0xFFFF;
		vehicle_update_showing_film(vehicle);
		break;
	case RIDE_MODE_CIRCUS_SHOW:
		vehicle->status = VEHICLE_STATUS_DOING_CIRCUS_SHOW;
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		vehicle->var_4C = 0xFFFF;
		vehicle_update_doing_circus_show(vehicle);
		break;
	case RIDE_MODE_SPACE_RINGS:
		vehicle->status = VEHICLE_STATUS_SPACE_RINGS_OPERATING;
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		vehicle->var_1F = 0;
		vehicle->var_4C = 0xFFFF;
		vehicle_update_space_rings_operating(vehicle);
		break;
	case RIDE_MODE_HAUNTED_HOUSE:
		vehicle->status = VEHICLE_STATUS_HAUNTED_HOUSE_OPERATING;
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		vehicle->var_1F = 0;
		vehicle->var_4C = 0xFFFF;
		vehicle_update_haunted_house_operating(vehicle);
		break;
	case RIDE_MODE_CROOKED_HOUSE:
		vehicle->status = VEHICLE_STATUS_CROOKED_HOUSE_OPERATING;
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		vehicle->var_1F = 0;
		vehicle->var_4C = 0xFFFF;
		vehicle_update_crooked_house_operating(vehicle);
		break;
	default:
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		vehicle->var_CE = 0;
		break;
	}
}

/* rct2: 0x006D9EB0 */
static void vehicle_peep_easteregg_here_we_are(rct_vehicle* vehicle) {
	uint16 spriteId = vehicle->sprite_index;
	do {
		vehicle = GET_VEHICLE(spriteId);
		for (int i = 0; i < vehicle->num_peeps; ++i) {
			rct_peep* peep = GET_PEEP(vehicle->peep[i]);
			if (peep->flags & PEEP_FLAGS_HERE_WE_ARE) {
				peep_insert_new_thought(peep, PEEP_THOUGHT_HERE_WE_ARE, peep->current_ride);
			}
		}
	} while ((spriteId = vehicle->next_vehicle_on_train) != 0xFFFF);
}

/* rct2: 0x006D7338
 * Performed when vehicle has completed a full circuit
 */
static void vehicle_update_test_finish(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);
	ride->lifecycle_flags &= ~RIDE_LIFECYCLE_TEST_IN_PROGRESS;
	vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_TESTING;
	ride->lifecycle_flags |= RIDE_LIFECYCLE_TESTED;

	for (int i = ride->num_stations - 1; i >= 1; i--) {
		if (ride->time[i - 1] != 0)
			continue;

		uint16 oldTime = ride->time[i - 1];
		ride->time[i - 1] = ride->time[i];
		ride->time[i] = oldTime;

		sint32 oldLength = ride->length[i - 1];
		ride->length[i - 1] = ride->length[i];
		ride->length[i] = oldLength;
	}

	uint32 totalTime = 0;
	for (uint8 i = 0; i < ride->num_stations; ++i) {
		totalTime += ride->time[i];
	}

	totalTime = min(totalTime, 1);
	ride->average_speed = ride->average_speed / totalTime;

	window_invalidate_by_number(WC_RIDE, vehicle->ride);
}

/* rct2: 0x006D6BE7
 *
 */
static void vehicle_test_reset(rct_vehicle* vehicle) {
	vehicle->update_flags |= VEHICLE_UPDATE_FLAG_TESTING;

	rct_ride* ride = GET_RIDE(vehicle->ride);
	ride->lifecycle_flags |= RIDE_LIFECYCLE_TEST_IN_PROGRESS;
	ride->lifecycle_flags &= ~RIDE_LIFECYCLE_NO_RAW_STATS;
	ride->max_speed = 0;
	ride->average_speed = 0;
	ride->current_test_segment = 0;
	ride->var_0E1 = 0;
	ride->max_positive_vertical_g = FIXED_2DP(1, 0);
	ride->max_negative_vertical_g = FIXED_2DP(1, 0);
	ride->max_lateral_g = 0;
	ride->previous_vertical_g = 0;
	ride->previous_lateral_g = 0;
	ride->testing_flags = 0;
	ride->var_10C = 0xFFFF;
	ride->var_11F = 0xFF;
	ride->turn_count_default = 0;
	ride->turn_count_banked = 0;
	ride->turn_count_sloped = 0;
	ride->inversions = 0;
	ride->drops = 0;
	ride->sheltered_length = 0;
	ride->var_11C = 0;
	ride->num_sheltered_sections = 0;
	ride->highest_drop_height = 0;
	ride->special_track_elements = 0;
	memset(&ride->length, 0, 4 * 4);
	memset(&ride->time, 0, 4 * 2);
	ride->var_1F6 = vehicle->current_station;
	window_invalidate_by_number(WC_RIDE, vehicle->ride);
}

static bool vehicle_next_tower_element_is_top(rct_vehicle* vehicle) {
	rct_map_element* mapElement = map_get_track_element_at_of_type(
		vehicle->track_x,
		vehicle->track_y,
		vehicle->track_z / 8,
		vehicle->track_type >> 2);

	if (mapElement->flags & MAP_ELEMENT_FLAG_LAST_TILE) {
		return true;
	}

	if (mapElement->clearance_height == (mapElement + 1)->base_height) {
		if ((mapElement + 1)->properties.track.type == TRACK_ELEM_TOWER_SECTION) {
			return false;
		}
	}

	if ((mapElement + 1)->flags & MAP_ELEMENT_FLAG_LAST_TILE) {
		return true;
	}

	if (mapElement->clearance_height != (mapElement + 2)->base_height) {
		return true;
	}

	if ((mapElement + 2)->properties.track.type == TRACK_ELEM_TOWER_SECTION) {
		return false;
	}
	return true;
}

/* rct2: 0x006D986C */
static void vehicle_update_travelling_boat_hire_setup(rct_vehicle* vehicle) {
	vehicle->var_34 = vehicle->sprite_direction;
	vehicle->track_x = vehicle->x & 0xFFE0;
	vehicle->track_y = vehicle->y & 0xFFE0;

	rct_xy8 location = {
		.x = (vehicle->track_x + RCT2_ADDRESS(0x00993CCC, sint16)[2 * (vehicle->sprite_direction >> 3)]) / 32,
		.y = (vehicle->track_y + RCT2_ADDRESS(0x00993CCE, sint16)[2 * (vehicle->sprite_direction >> 3)]) / 32
	};

	vehicle->boat_location = location;
	vehicle->var_35 = 0;
	vehicle->status = VEHICLE_STATUS_TRAVELLING_BOAT;
	vehicle_invalidate_window(vehicle);
	vehicle->sub_state = 0;
	vehicle->var_24 += 27924;

	vehicle_update_travelling_boat(vehicle);
}

/* rct2: 0x006D982F */
static void vehicle_update_departing_boat_hire(rct_vehicle* vehicle) {
	vehicle->lost_time_out = 0;
	rct_ride* ride = GET_RIDE(vehicle->ride);

	ride->station_depart[vehicle->current_station] &= STATION_DEPART_FLAG;
	uint8 waitingTime = max(ride->min_waiting_time, 3);
	waitingTime = min(waitingTime, 127);
	ride->station_depart[vehicle->current_station] |= waitingTime;
	vehicle_update_travelling_boat_hire_setup(vehicle);
}

/* rct2: 0x006D845B */
static void vehicle_update_departing(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);
	rct_ride_type* rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);

	if (vehicle->sub_state == 0) {
		if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_BROKEN_TRAIN) {
			if (ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN)
				return;

			ride->lifecycle_flags |= RIDE_LIFECYCLE_BROKEN_DOWN;
			ride_breakdown_add_news_item(vehicle->ride);

			ride->window_invalidate_flags |=
				RIDE_INVALIDATE_RIDE_MAIN |
				RIDE_INVALIDATE_RIDE_LIST |
				RIDE_INVALIDATE_RIDE_MAINTENANCE;
			ride->inspection_station = vehicle->current_station;
			ride->breakdown_reason = ride->breakdown_reason_pending;
			vehicle->velocity = 0;
			return;
		}

		vehicle->sub_state = 1;
		vehicle_peep_easteregg_here_we_are(vehicle);

		if (rideEntry->flags & RIDE_ENTRY_FLAG_3) {
			uint8 soundId = (rideEntry->vehicles[0].sound_range == 4) ?
				SOUND_TRAM :
				SOUND_TRAIN_CHUGGING;

			audio_play_sound_at_location(
				soundId,
				vehicle->x,
				vehicle->y,
				vehicle->z);
		}

		if (ride->mode == RIDE_MODE_UPWARD_LAUNCH ||
			(ride->mode == RIDE_MODE_DOWNWARD_LAUNCH && vehicle->var_CE > 1)) {

			audio_play_sound_at_location(
				SOUND_RIDE_LAUNCH_2,
				vehicle->x,
				vehicle->y,
				vehicle->z
			);
		}

		if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_TESTED)) {
			if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_TESTING) {
				if (ride->current_test_segment + 1 < ride->num_stations) {
					ride->current_test_segment++;
					ride->var_1F6 = vehicle->current_station;
				}
				else {
					vehicle_update_test_finish(vehicle);
				}
			}
			else if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_TEST_IN_PROGRESS)) {
				vehicle_test_reset(vehicle);
			}
		}
	}

	rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

	switch (ride->mode) {
	case RIDE_MODE_REVERSE_INCLINE_LAUNCHED_SHUTTLE:
		if (vehicle->velocity >= -131940)
			vehicle->var_2C = -3298;
		break;
	case RIDE_MODE_POWERED_LAUNCH_PASSTROUGH:
	case RIDE_MODE_POWERED_LAUNCH:
	case RIDE_MODE_POWERED_LAUNCH_BLOCK_SECTIONED:
	case RIDE_MODE_LIM_POWERED_LAUNCH:
	case RIDE_MODE_UPWARD_LAUNCH:
		if (ride->type == RIDE_TYPE_AIR_POWERED_VERTICAL_COASTER) {
			if ((ride->launch_speed << 16) > vehicle->velocity) {
				vehicle->var_2C = ride->launch_speed << 13;
			}
			break;
		}

		if ((ride->launch_speed << 16) > vehicle->velocity)
			vehicle->var_2C = ride->launch_speed << 12;
		break;
	case RIDE_MODE_DOWNWARD_LAUNCH:
		if (vehicle->var_CE >= 1) {
			if ((14 << 16) > vehicle->velocity)
				vehicle->var_2C = 14 << 12;
			break;
		}
		// Fall through
	case RIDE_MODE_CONTINUOUS_CIRCUIT:
	case RIDE_MODE_CONTINUOUS_CIRCUIT_BLOCK_SECTIONED:
	case RIDE_MODE_ROTATING_LIFT:
	case RIDE_MODE_FREEFALL_DROP:
	case RIDE_MODE_BOAT_HIRE:
		if (vehicleEntry->var_14 & (1 << 3))
			break;

		if (vehicle->velocity <= 131940)
				vehicle->var_2C = 3298;
		break;
	}

	uint32 flags = vehicle_update_track_motion(vehicle, NULL);

	if (flags & (1 << 8)) {
		if (ride->mode == RIDE_MODE_REVERSE_INCLINE_LAUNCHED_SHUTTLE) {
			vehicle->velocity = -vehicle->velocity;
			vehicle_finish_departing(vehicle);
			return;
		}
	}

	if (flags & ((1 << 5) | (1 << 12))) {
		if (ride->mode == RIDE_MODE_BOAT_HIRE) {
			vehicle_update_departing_boat_hire(vehicle);
			return;
		}
		else if (ride->mode == RIDE_MODE_REVERSE_INCLINE_LAUNCHED_SHUTTLE) {
			vehicle->velocity = -vehicle->velocity;
			vehicle_finish_departing(vehicle);
			return;
		}
		else if (ride->mode == RIDE_MODE_SHUTTLE) {
			vehicle->update_flags ^= VEHICLE_UPDATE_FLAG_3;
			vehicle->velocity = 0;
		}
	}

	if (flags & (1 << 4)) {
		vehicle->var_B8 |= (1 << 1);
		if (ride->mode != RIDE_MODE_REVERSE_INCLINE_LAUNCHED_SHUTTLE) {
			sint32 speed = ride->lift_hill_speed * 31079;
			if (vehicle->velocity <= speed) {
				vehicle->var_2C = 15539;
				if (vehicle->velocity != 0) {
					if (RCT2_GLOBAL(0x00F64E34, uint8) == BREAKDOWN_SAFETY_CUT_OUT) {
						vehicle->update_flags |= VEHICLE_UPDATE_FLAG_7;
						vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_1;
					}
				}
				else
					vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_1;
			}
		}
		else {
			sint32 speed = ride->lift_hill_speed * -31079;
			if (vehicle->velocity >= speed) {
				vehicle->var_2C = -15539;
				if (vehicle->velocity != 0) {
					if (RCT2_GLOBAL(0x00F64E34, uint8) == BREAKDOWN_SAFETY_CUT_OUT) {
						vehicle->update_flags |= VEHICLE_UPDATE_FLAG_7;
						vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_1;
					}
				}
				else
					vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_1;
			}
		}
	}

	if (ride->mode == RIDE_MODE_FREEFALL_DROP) {
		vehicle->var_C5++;
	}else{
		bool shouldLaunch = true;
		if (ride->mode == RIDE_MODE_DOWNWARD_LAUNCH) {
			if (vehicle->var_CE < 1)
				shouldLaunch = false;
		}

		if (shouldLaunch) {
			if (!(flags & (1 << 3)) || (RCT2_GLOBAL(0x00F64E1C, uint8) != vehicle->current_station)) {
				vehicle_finish_departing(vehicle);
				return;
			}

			if (!(flags & (1 << 5)))
				return;
			if (ride->mode == RIDE_MODE_BOAT_HIRE ||
				ride->mode == RIDE_MODE_ROTATING_LIFT ||
				ride->mode == RIDE_MODE_SHUTTLE)
				return;

			if (ride->lifecycle_flags & RIDE_LIFECYCLE_SIX_FLAGS_DEPRECATED)
				return;

			vehicle_update_crash_setup(vehicle);
			return;
		}
	}

	if (vehicle_next_tower_element_is_top(vehicle) == false) {
		if (ride->mode == RIDE_MODE_FREEFALL_DROP)
			invalidate_sprite_2((rct_sprite*)vehicle);
		return;
	}

	vehicle_finish_departing(vehicle);
}

/* rct2: 0x006D8858
 * Part of vehicle_update_departing
 * Called after finishing departing sequence to enter
 * traveling state.
 * Vertical rides class the lift to the top of the tower
 * as the departing sequence. After this point station
 * boosters do not affect the ride.
 */
static void vehicle_finish_departing(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);

	if (ride->mode == RIDE_MODE_DOWNWARD_LAUNCH ) {
		if (vehicle->var_CE >= 1 && (14 << 16) > vehicle->velocity)
			return;

		audio_play_sound_at_location(
			SOUND_RIDE_LAUNCH_1,
			vehicle->x,
			vehicle->y,
			vehicle->z);
	}

	if (ride->mode == RIDE_MODE_UPWARD_LAUNCH) {
		if ((ride->launch_speed << 16) > vehicle->velocity)
			return;

		audio_play_sound_at_location(
			SOUND_RIDE_LAUNCH_1,
			vehicle->x,
			vehicle->y,
			vehicle->z);
	}

	if (ride->mode != RIDE_MODE_RACE &&
		ride->mode != RIDE_MODE_CONTINUOUS_CIRCUIT_BLOCK_SECTIONED &&
		ride->mode != RIDE_MODE_POWERED_LAUNCH_BLOCK_SECTIONED) {

		ride->station_depart[vehicle->current_station] &= STATION_DEPART_FLAG;
		uint8 waitingTime = 3;
		if (ride->depart_flags & RIDE_DEPART_WAIT_FOR_MINIMUM_LENGTH) {
			waitingTime = max(ride->min_waiting_time, 3);
			waitingTime = min(waitingTime, 127);
		}

		ride->station_depart[vehicle->current_station] |= waitingTime;
	}

	vehicle->status = VEHICLE_STATUS_TRAVELLING;
	vehicle_invalidate_window(vehicle);
	vehicle->lost_time_out = 0;

	vehicle->sub_state = 1;
	if (vehicle->velocity < 0)
		vehicle->sub_state = 0;
}

/* rct2: 0x006DE5CB */
static void vehicle_check_if_missing(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);

	if (ride->lifecycle_flags & (RIDE_LIFECYCLE_BROKEN_DOWN | RIDE_LIFECYCLE_CRASHED))
		return;

	if (ride->mode == RIDE_MODE_CONTINUOUS_CIRCUIT_BLOCK_SECTIONED ||
		ride->mode == RIDE_MODE_POWERED_LAUNCH_BLOCK_SECTIONED)
		return;

	if (!ride_type_has_flag(ride->type, RIDE_TYPE_FLAG_27))
		return;

	vehicle->lost_time_out++;
	if (ride->lifecycle_flags & RIDE_LIFECYCLE_11)
		return;

	uint16 limit = ride->type == RIDE_TYPE_BOAT_RIDE ? 15360 : 9600;

	if (vehicle->lost_time_out <= limit)
		return;

	ride->lifecycle_flags |= RIDE_LIFECYCLE_11;

	RCT2_GLOBAL(0x0013CE952, rct_string_id)
		= RCT2_ADDRESS(0x0097C98E, rct_string_id)[ride->type * 4] + 6;

	uint8 vehicleIndex = 0;
	for (; vehicleIndex < ride->num_vehicles; ++vehicleIndex)
		if (ride->vehicles[vehicleIndex] == vehicle->sprite_index) break;

	vehicleIndex++;
	RCT2_GLOBAL(0x0013CE954, uint16) = vehicleIndex;
	RCT2_GLOBAL(0x0013CE956, rct_string_id) = ride->name;
	RCT2_GLOBAL(0x0013CE958, uint32) = ride->name_arguments;
	RCT2_GLOBAL(0x0013CE95C, rct_string_id) =
		RCT2_ADDRESS(0x0097C98E, rct_string_id)[ride->type * 4 + 2];

	news_item_add_to_queue(NEWS_ITEM_RIDE, 2218, vehicle->ride);
}

/**
 * Setup function for a vehicle colliding with 
 * another vehicle.
 *
 *  rct2: 0x006DA059
 */
static void vehicle_update_collision_setup(rct_vehicle* vehicle) {
	vehicle->status = VEHICLE_STATUS_CRASHED;
	vehicle_invalidate_window(vehicle);

	rct_ride* ride = GET_RIDE(vehicle->ride);
	if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_CRASHED)) {
		rct_vehicle* frontVehicle = vehicle;
		while (frontVehicle->is_child != 0)frontVehicle = GET_VEHICLE(frontVehicle->prev_vehicle_on_ride);

		uint8 trainIndex = 0;
		while (ride->vehicles[trainIndex] != frontVehicle->sprite_index)
			trainIndex++;

		ride_crash(vehicle->ride, trainIndex);

		if (ride->status != RIDE_STATUS_CLOSED) {
			ride_set_status(vehicle->ride, RIDE_STATUS_CLOSED);
		}
	}

	ride->lifecycle_flags |= RIDE_LIFECYCLE_CRASHED;
	ride->window_invalidate_flags |= RIDE_INVALIDATE_RIDE_MAIN | RIDE_INVALIDATE_RIDE_LIST;
	vehicle_kill_all_passengers(vehicle);

	rct_vehicle* lastVehicle = vehicle;
	uint16 spriteId = vehicle->sprite_index;
	for (rct_vehicle* train; spriteId != 0xFFFF; spriteId = train->next_vehicle_on_train) {
		train = GET_VEHICLE(spriteId);
		lastVehicle = train;

		train->sub_state = 2;

		audio_play_sound_at_location(
			SOUND_CRASH,
			train->x,
			train->y,
			train->z
			);

		sprite_misc_3_create(
			train->x,
			train->y,
			train->z
			);

		for (int i = 0; i < 10; i++) {
			crashed_vehicle_particle_create(
				train->colours,
				train->x,
				train->y,
				train->z
				);
		}

		train->var_0C |= (1 << 7);
		train->var_C8 = scenario_rand();
		train->var_CA = scenario_rand();

		train->var_C5 = train->var_CA & 0x7;
		train->sprite_width = 13;
		train->sprite_height_negative = 45;
		train->sprite_height_positive = 5;

		sprite_move(train->x, train->y, train->z, (rct_sprite*)train);
		invalidate_sprite_2((rct_sprite*)train);

		train->var_4E = 0;
	}

	(GET_VEHICLE(vehicle->prev_vehicle_on_ride))->next_vehicle_on_ride = lastVehicle->next_vehicle_on_ride;
	(GET_VEHICLE(lastVehicle->next_vehicle_on_ride))->prev_vehicle_on_ride = vehicle->prev_vehicle_on_ride;
	vehicle->velocity = 0;
}

/* rct2: 0x006D9EFE */
static void vehicle_update_crash_setup(rct_vehicle* vehicle) {
	vehicle->status = VEHICLE_STATUS_CRASHING;
	vehicle_invalidate_window(vehicle);

	int num_peeps = vehicle_get_total_num_peeps(vehicle);
	if (num_peeps != 0) {
		audio_play_sound_at_location(
			SOUND_HAUNTED_HOUSE_SCREAM_2,
			vehicle->x,
			vehicle->y,
			vehicle->z
			);
	}

	int edx = vehicle->velocity >> 10;

	rct_vehicle* lastVehicle = vehicle;
	uint16 spriteId = vehicle->sprite_index;
	for (rct_vehicle* trainVehicle; spriteId != 0xFFFF; spriteId = trainVehicle->next_vehicle_on_train) {
		trainVehicle = GET_VEHICLE(spriteId);
		lastVehicle = trainVehicle;

		trainVehicle->sub_state = 0;
		int x = RCT2_ADDRESS(0x009A3AC4, sint16)[trainVehicle->sprite_direction * 2];
		int	y = RCT2_ADDRESS(0x009A3AC6, sint16)[trainVehicle->sprite_direction * 2];

		int ecx = RCT2_ADDRESS(0x009A37E4, uint32)[trainVehicle->var_1F] >> 15;
		x *= ecx;
		y *= ecx;
		x >>= 16;
		y >>= 16;
		ecx = RCT2_ADDRESS(0x009A38D4, uint32)[trainVehicle->var_1F] >> 23;
		x *= edx;
		y *= edx;
		ecx *= edx;
		x >>= 8;
		y >>= 8;
		ecx >>= 8;

		trainVehicle->var_B6 = x;
		trainVehicle->var_C0 = y;
		trainVehicle->var_4E = ecx;
		trainVehicle->var_B6 += (scenario_rand() & 0xF) - 8;
		trainVehicle->var_C0 += (scenario_rand() & 0xF) - 8;
		trainVehicle->var_4E += (scenario_rand() & 0xF) - 8;

		trainVehicle->track_x = 0;
		trainVehicle->track_y = 0;
		trainVehicle->track_z = 0;
	}

	(GET_VEHICLE(vehicle->prev_vehicle_on_ride))->next_vehicle_on_ride = lastVehicle->next_vehicle_on_ride;
	(GET_VEHICLE(lastVehicle->next_vehicle_on_ride))->prev_vehicle_on_ride = vehicle->prev_vehicle_on_ride;
	vehicle->velocity = 0;
}

/* rct2: 0x006D8937 */
static void vehicle_update_travelling(rct_vehicle* vehicle) {
	vehicle_check_if_missing(vehicle);

	rct_ride* ride = GET_RIDE(vehicle->ride);
	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0 && ride->mode == RIDE_MODE_ROTATING_LIFT)
		return;

	if (vehicle->sub_state == 2) {
		vehicle->velocity = 0;
		vehicle->var_2C = 0;
		vehicle->var_C0--;
		if (vehicle->var_C0 == 0)
			vehicle->sub_state = 0;
	}

	if (ride->mode == RIDE_MODE_FREEFALL_DROP &&
		vehicle->var_C5 != 0) {

		vehicle->var_C5++;
		vehicle->velocity = 0;
		vehicle->var_2C = 0;
		invalidate_sprite_2((rct_sprite*)vehicle);
		return;
	}

	uint32 flags = vehicle_update_track_motion(vehicle, NULL);

	bool skipCheck = false;
	if (flags & ((1 << 8) | (1 << 9)) &&
		ride->mode == RIDE_MODE_REVERSE_INCLINE_LAUNCHED_SHUTTLE &&
		vehicle->sub_state == 0) {
		vehicle->sub_state = 1;
		vehicle->velocity = 0;
		skipCheck = true;
	}

	if (!skipCheck) {
		if (flags & (1 << 6)) {
			vehicle_update_crash_setup(vehicle);
			return;
		}

		if (flags & (1 << 7)) {
			vehicle_update_collision_setup(vehicle);
			return;
		}

		if (flags & ((1 << 5) | (1 << 12))) {
			if (ride->mode == RIDE_MODE_ROTATING_LIFT) {
				if (vehicle->sub_state <= 1) {
					vehicle->status = VEHICLE_STATUS_ARRIVING;
					vehicle_invalidate_window(vehicle);
					vehicle->sub_state = 1;
					vehicle->var_C0 = 0;
					return;
				}
			}
			else if (ride->mode == RIDE_MODE_BOAT_HIRE) {
				vehicle_update_travelling_boat_hire_setup(vehicle);
				return;
			}
			else if (ride->mode == RIDE_MODE_SHUTTLE) {
				vehicle->update_flags ^= VEHICLE_UPDATE_FLAG_3;
				vehicle->velocity = 0;
			}
			else {
				if (ride->lifecycle_flags & RIDE_LIFECYCLE_SIX_FLAGS_DEPRECATED) {
					if (vehicle->sub_state != 0) {
						vehicle->update_flags ^= VEHICLE_UPDATE_FLAG_3;
						vehicle->velocity = 0;
					}
					else {
						vehicle->sub_state = 1;
						vehicle->velocity = 0;
					}
				}
				else {
					if (vehicle->sub_state != 0) {
						vehicle_update_crash_setup(vehicle);
						return;
					}
					vehicle->sub_state = 1;
					vehicle->velocity = 0;
				}
			}
		}
	}

	if (ride->mode == RIDE_MODE_ROTATING_LIFT &&
		vehicle->sub_state <= 1) {

		if (vehicle->sub_state == 0) {
			if (vehicle->velocity >= -131940)
				vehicle->var_2C = -3298;
			vehicle->velocity = max(vehicle->velocity, -131940);
		}
		else {
			if (vehicle_next_tower_element_is_top(vehicle) == true) {
				vehicle->velocity = 0;
				vehicle->sub_state = 2;
				vehicle->var_C0 = 150;
			}
			else {
				if (vehicle->velocity <= 131940)
					vehicle->var_2C = 3298;
			}
		}
	}

	if (flags & (1 << 4)) {
		if (ride->mode == RIDE_MODE_REVERSE_INCLINE_LAUNCHED_SHUTTLE) {
			if (vehicle->sub_state == 0) {
				if (vehicle->velocity != 0)
					vehicle->var_B8 |= (1 << 1);

				if (!(vehicle->update_flags & VEHICLE_UPDATE_FLAG_12)) {
					if (vehicle->velocity >= ride->lift_hill_speed * -31079) {
						vehicle->var_2C = -15539;

						if (RCT2_GLOBAL(0x00F64E34, uint8) == 0) {
							vehicle->var_B8 &= ~(1 << 1);
							vehicle->update_flags |= VEHICLE_UPDATE_FLAG_7;
						}
					}
				}
			}
		}
		else {
			vehicle->var_B8 |= (1 << 1);
			if (vehicle->velocity <= ride->lift_hill_speed * 31079) {
				vehicle->var_2C = 15539;
				if (vehicle->velocity != 0) {
					if (RCT2_GLOBAL(0x00F64E34, uint8) == 0) {
						vehicle->update_flags |= VEHICLE_UPDATE_FLAG_7;
						vehicle->var_B8 &= ~(1 << 1);
					}
				}
				else {
					vehicle->var_B8 &= ~(1 << 1);
				}
			}
		}
	}

	if (!(flags & (1 << 3)))
		return;

	if (ride->mode == RIDE_MODE_REVERSE_INCLINE_LAUNCHED_SHUTTLE &&
		vehicle->velocity >= 0 &&
		!(vehicle->update_flags & VEHICLE_UPDATE_FLAG_12)) {
		return;
	}

	if (ride->mode == RIDE_MODE_POWERED_LAUNCH_PASSTROUGH &&
		vehicle->velocity < 0)
		return;

	vehicle->status = VEHICLE_STATUS_ARRIVING;
	vehicle->current_station = RCT2_GLOBAL(0x00F64E1C, uint8);
	vehicle_invalidate_window(vehicle);
	vehicle->var_C0 = 0;

	vehicle->sub_state = 0;
	if (vehicle->velocity < 0)
		vehicle->sub_state = 1;
}

/* rct2: 0x006D8C36 */
static void vehicle_update_arriving(rct_vehicle* vehicle) {
	RCT2_GLOBAL(0x00F64E35, uint8) = 1;
	rct_ride* ride = GET_RIDE(vehicle->ride);

	switch (ride->mode) {
	case RIDE_MODE_SWING:
	case RIDE_MODE_ROTATION:
	case RIDE_MODE_FORWARD_ROTATION:
	case RIDE_MODE_BACKWARD_ROTATION:
	case RIDE_MODE_FILM_AVENGING_AVIATORS:
	case RIDE_MODE_FILM_THRILL_RIDERS:
	case RIDE_MODE_BEGINNERS:
	case RIDE_MODE_INTENSE:
	case RIDE_MODE_BERSERK:
	case RIDE_MODE_3D_FILM_MOUSE_TAILS:
	case RIDE_MODE_3D_FILM_STORM_CHASERS:
	case RIDE_MODE_3D_FILM_SPACE_RAIDERS:
	case RIDE_MODE_CIRCUS_SHOW:
	case RIDE_MODE_SPACE_RINGS:
	case RIDE_MODE_HAUNTED_HOUSE:
	case RIDE_MODE_CROOKED_HOUSE:
		vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_12;
		vehicle->velocity = 0;
		vehicle->var_2C = 0;
		vehicle->status = VEHICLE_STATUS_UNLOADING_PASSENGERS;
		vehicle->sub_state = 0;
		vehicle_invalidate_window(vehicle);
		return;
	}

	if (ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN &&
		ride->breakdown_reason_pending == BREAKDOWN_BRAKES_FAILURE &&
		ride->inspection_station == vehicle->current_station &&
		ride->mechanic_status != RIDE_MECHANIC_STATUS_4)
		RCT2_GLOBAL(0x00F64E35, uint8) = 0;

	rct_ride_type* rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);
	rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

	if (vehicle->sub_state == 0) {
		if (ride->mode == RIDE_MODE_RACE &&
			ride->lifecycle_flags & RIDE_LIFECYCLE_PASS_STATION_NO_STOPPING) {
			goto loc_6D8E36;
		}

		if (vehicle->velocity <= 131940) {
			vehicle->var_2C = 3298;
			goto loc_6D8E36;
		}

		sint32 velocity_diff = vehicle->velocity;
		if (velocity_diff >= 1572864)
			velocity_diff /= 8;
		else
			velocity_diff /= 16;

		if (RCT2_GLOBAL(0x00F64E35, uint8) == 0) {
			goto loc_6D8E36;
		}

		if (ride->num_circuits != 1) {
			if (vehicle->num_laps + 1 < ride->num_circuits) {
				goto loc_6D8E36;
			}
		}
		vehicle->velocity -= velocity_diff;
		vehicle->var_2C = 0;
	}
	else {
		if (!(vehicleEntry->var_14 & (1 << 3)) &&
			vehicle->velocity >= -131940) {
			vehicle->var_2C = -3298;
		}

		if (vehicle->velocity >= -131940) {
			goto loc_6D8E36;
		}

		sint32 velocity_diff = vehicle->velocity;
		if (velocity_diff < -1572864)
			velocity_diff /= 8;
		else
			velocity_diff /= 16;

		if (RCT2_GLOBAL(0x00F64E35, uint8) == 0) {
			goto loc_6D8E36;
		}

		if (vehicle->num_laps + 1 < ride->num_circuits) {
			goto loc_6D8E36;
		}

		if (vehicle->num_laps + 1 != ride->num_circuits) {
			vehicle->velocity -= velocity_diff;
			vehicle->var_2C = 0;
			goto loc_6D8E36;
		}

		if (RideData4[ride->type].flags & RIDE_TYPE_FLAG4_ALLOW_MULTIPLE_CIRCUITS &&
			ride->mode != RIDE_MODE_SHUTTLE &&
			ride->mode != RIDE_MODE_POWERED_LAUNCH) {
			vehicle->update_flags |= VEHICLE_UPDATE_FLAG_12;
		}
		else{
			vehicle->velocity -= velocity_diff;
			vehicle->var_2C = 0;
		}
	}

	uint32 flags;
 loc_6D8E36:
	flags = vehicle_update_track_motion(vehicle, NULL);
	if (flags & (1 << 7) &&
		RCT2_GLOBAL(0x00F64E35, uint8) == 0) {
		vehicle_update_collision_setup(vehicle);
		return;
	}

	if (flags & (1 << 0) &&
		RCT2_GLOBAL(0x00F64E35, uint8) == 0) {
		vehicle->status = VEHICLE_STATUS_DEPARTING;
		vehicle->sub_state = 1;
		vehicle_invalidate_window(vehicle);
		return;
	}

	if (!(flags & ((1 << 0) | (1 << 1) | (1 << 5)))) {
		if (vehicle->velocity > 98955)
			vehicle->var_C0 = 0;
		return;
	}

	vehicle->var_C0++;
	if (flags & (1 << 1) &&
		vehicleEntry->var_14 & (1 << 14) &&
		vehicle->var_C0 < 40){
		return;
	}

	rct_map_element* mapElement = map_get_track_element_at(
		vehicle->track_x,
		vehicle->track_y,
		vehicle->track_z / 8
		);

	vehicle->current_station = map_get_station(mapElement);
	vehicle->num_laps++;

	if (vehicle->sub_state != 0) {
		if (vehicle->num_laps < ride->num_circuits) {
			vehicle->status = VEHICLE_STATUS_DEPARTING;
			vehicle->sub_state = 1;
			vehicle_invalidate_window(vehicle);
			return;
		}

		if (vehicle->num_laps == ride->num_circuits &&
			vehicle->update_flags & VEHICLE_UPDATE_FLAG_12) {
			vehicle->status = VEHICLE_STATUS_DEPARTING;
			vehicle->sub_state = 1;
			vehicle_invalidate_window(vehicle);
			return;
		}
	}

	if (ride->num_circuits != 1 &&
		vehicle->num_laps < ride->num_circuits) {
		vehicle->status = VEHICLE_STATUS_DEPARTING;
		vehicle->sub_state = 1;
		vehicle_invalidate_window(vehicle);
		return;
	}

	if ((ride->mode == RIDE_MODE_UPWARD_LAUNCH ||
		ride->mode == RIDE_MODE_DOWNWARD_LAUNCH) &&
		vehicle->var_CE < 2) {
		audio_play_sound_at_location(
			SOUND_RIDE_LAUNCH_2,
			vehicle->x,
			vehicle->y,
			vehicle->z);
		vehicle->velocity = 0;
		vehicle->var_2C = 0;
		vehicle->status = VEHICLE_STATUS_DEPARTING;
		vehicle->sub_state = 1;
		vehicle_invalidate_window(vehicle);
		return;
	}

	if (ride->mode == RIDE_MODE_RACE &&
		ride->lifecycle_flags & RIDE_LIFECYCLE_PASS_STATION_NO_STOPPING) {
		vehicle->status = VEHICLE_STATUS_DEPARTING;
		vehicle->sub_state = 1;
		vehicle_invalidate_window(vehicle);
		return;
	}

	vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_12;
	vehicle->velocity = 0;
	vehicle->var_2C = 0;
	vehicle->status = VEHICLE_STATUS_UNLOADING_PASSENGERS;
	vehicle->sub_state = 0;
	vehicle_invalidate_window(vehicle);
}

/* rct2: 0x006D9002 */
static void vehicle_update_unloading_passengers(rct_vehicle* vehicle) {
	if (vehicle->sub_state == 0) {
		if (!vehicle_open_restraints(vehicle)) {
			vehicle->sub_state = 1;
		}
	}

	rct_ride* ride = GET_RIDE(vehicle->ride);
	if (ride->mode == RIDE_MODE_FORWARD_ROTATION ||
		ride->mode == RIDE_MODE_BACKWARD_ROTATION) {
		uint8 seat = ((-vehicle->var_1F) >> 3) & 0xF;
		if (vehicle->restraints_position == 255 &&
			(vehicle->peep[seat * 2] != 0xFFFF)) {
			vehicle->next_free_seat -= 2;

			rct_peep* peep = GET_PEEP(vehicle->peep[seat * 2]);
			vehicle->peep[seat * 2] = 0xFFFF;

			peep_decrement_num_riders(peep);
			peep->sub_state = 7;
			peep->state = PEEP_STATE_LEAVING_RIDE;
			peep_window_state_update(peep);

			peep = GET_PEEP(vehicle->peep[seat * 2 + 1]);
			vehicle->peep[seat * 2 + 1] = 0xFFFF;

			peep_decrement_num_riders(peep);
			peep->sub_state = 7;
			peep->state = PEEP_STATE_LEAVING_RIDE;
			peep_window_state_update(peep);
		}
	}
	else {
		if (ride->exits[vehicle->current_station] == 0xFFFF) {
			if (vehicle->sub_state != 1)
				return;

			if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_TESTED) &&
				vehicle->update_flags & VEHICLE_UPDATE_FLAG_TESTING &&
				ride->current_test_segment + 1 >= ride->num_stations) {
				vehicle_update_test_finish(vehicle);
			}
			vehicle->status = VEHICLE_STATUS_MOVING_TO_END_OF_STATION;
			vehicle->sub_state = 0;
			vehicle_invalidate_window(vehicle);
			return;
		}

		uint16 spriteId = vehicle->sprite_index;
		for (rct_vehicle* train; spriteId != 0xFFFF; spriteId = train->next_vehicle_on_train) {
			train = GET_VEHICLE(spriteId);
			if (train->restraints_position != 255)
				continue;

			if (train->next_free_seat == 0)
				continue;

			train->next_free_seat = 0;
			for (uint8 peepIndex = 0; peepIndex < train->num_peeps; peepIndex++) {
				rct_peep* peep = GET_PEEP(train->peep[peepIndex]);
				peep_decrement_num_riders(peep);
				peep->sub_state = 7;
				peep->state = PEEP_STATE_LEAVING_RIDE;
				peep_window_state_update(peep);
			}
		}
	}

	if (vehicle->sub_state != 1)
		return;

	uint16 spriteId = vehicle->sprite_index;
	for (rct_vehicle* train; spriteId != 0xFFFF; spriteId = train->next_vehicle_on_train) {
		train = GET_VEHICLE(spriteId);
		if (train->num_peeps != train->next_free_seat)
			return;
	}

	if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_TESTED) &&
		vehicle->update_flags & VEHICLE_UPDATE_FLAG_TESTING &&
		ride->current_test_segment + 1 >= ride->num_stations) {
		vehicle_update_test_finish(vehicle);
	}
	vehicle->status = VEHICLE_STATUS_MOVING_TO_END_OF_STATION;
	vehicle->sub_state = 0;
	vehicle_invalidate_window(vehicle);
}

/* rct2: 0x006D9820 */
static void vehicle_update_travelling_boat(rct_vehicle* vehicle) {
	vehicle_check_if_missing(vehicle);
	RCT2_CALLPROC_X(0x006DA717, 0, 0, 0, 0, (int)vehicle, 0, 0);
}

/* rct2: 0x006D9249 */
static void vehicle_update_swinging(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);
	rct_ride_type* rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);

	// SubState for this ride means swinging state
	// 0 == first swing
	// 3 == full swing
	uint8 swingState = vehicle->sub_state;
	if (rideEntry->flags & RIDE_ENTRY_FLAG_ALTERNATIVE_SWING_MODE_1) {
		swingState += 4;
		if (rideEntry->flags & RIDE_ENTRY_FLAG_ALTERNATIVE_SWING_MODE_2)
			swingState += 4;
	}
	uint8* edi = RCT2_ADDRESS(0x0099F9D0, uint8*)[swingState];
	uint8 al = edi[(uint16)(vehicle->var_4C + 1)];

	// 0x80 indicates that a complete swing has been
	// completed and the next swing can start
	if (al != 0x80) {
		vehicle->var_4C++;
		if (al == vehicle->var_1F)
			return;
		// Used to know which sprite to draw
		vehicle->var_1F = al;
		invalidate_sprite_2((rct_sprite*)vehicle);
		return;
	}

	vehicle->var_4C = 0xFFFF;
	vehicle->var_CE++;
	if (ride->status != RIDE_STATUS_CLOSED) {
		// It takes 3 swings to get into full swing
		// ride->rotations already takes this into account
		if (vehicle->var_CE + 3 < ride->rotations) {
			// Go to the next swing state until we
			// are at full swing.
			if (vehicle->sub_state != 3) {
				vehicle->sub_state++;
			}
			vehicle_update_swinging(vehicle);
			return;
		}
	}

	// To get to this part of the code the
	// swing has to be in slowing down phase
	if (vehicle->sub_state == 0) {
		vehicle->status = VEHICLE_STATUS_ARRIVING;
		vehicle_invalidate_window(vehicle);
		vehicle->sub_state = 0;
		vehicle->var_C0 = 0;
		return;
	}
	// Go towards first swing state
	vehicle->sub_state--;
	vehicle_update_swinging(vehicle);
}

/* rct2: 0x006D9413 */
static void vehicle_update_ferris_wheel_rotating(rct_vehicle* vehicle) {
	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0)
		return;

	rct_ride* ride = GET_RIDE(vehicle->ride);
	if (((vehicle->var_4C -= 0x100) & 0xFF00) != 0)
		return;

	sint8 var_4C = vehicle->var_4C & 0xFF;

	if (var_4C == 3) {
		vehicle->var_4C = (0xFF & var_4C) | (0xFF00 & (var_4C << 8));
	}
	else if (var_4C < 3) {
		if (var_4C != -8)
			var_4C--;
		vehicle->var_4C = (0xFF & var_4C) | (0xFF00 & (-var_4C << 8));
	}
	else {
		var_4C--;
		vehicle->var_4C = (0xFF & var_4C) | (0xFF00 & (var_4C << 8));
	}

	uint8 rotation = vehicle->var_1F;
	if (ride->mode & RIDE_MODE_FORWARD_ROTATION)
		rotation++;
	else
		rotation--;

	rotation &= 0x7F;
	vehicle->var_1F = rotation;

	if (rotation == vehicle->sub_state)
		vehicle->var_CE++;

	invalidate_sprite_2((rct_sprite*)vehicle);

	uint8 subState = vehicle->sub_state;
	if (ride->mode & RIDE_MODE_FORWARD_ROTATION)
		subState++;
	else
		subState--;
	subState &= 0x7F;

	if (subState == vehicle->var_1F) {
		bool shouldStop = true;
		if (ride->status != RIDE_STATUS_CLOSED) {
			if (vehicle->var_CE <= ride->rotations)
				shouldStop = false;
		}

		if (shouldStop) {
			var_4C = (sint8)(vehicle->var_4C & 0xFF);
			vehicle->var_4C &= 0xFF00;
			vehicle->var_4C |= 0xFF & (-abs(var_4C));
		}
	}

	if ((sint8)(vehicle->var_4C & 0xFF) != -8)
		return;

	subState = vehicle->sub_state;
	if (ride->mode & RIDE_MODE_FORWARD_ROTATION)
		subState += 8;
	else
		subState -= 8;
	subState &= 0x7F;

	if (subState != vehicle->var_1F)
		return;

	vehicle->status = VEHICLE_STATUS_ARRIVING;
	vehicle_invalidate_window(vehicle);
	vehicle->sub_state = 0;
	vehicle->var_C0 = 0;
}

/* rct2: 0x006D94F2 */
static void vehicle_update_simulator_operating(rct_vehicle* vehicle) {
	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0)
		return;

	uint8* edi = RCT2_ADDRESS(0x009A042C, uint8*)[vehicle->sub_state];

	uint8 al = edi[(uint16)(vehicle->var_4C + 1)];
	if (al != 0xFF) {
		vehicle->var_4C++;
		if (al == vehicle->var_1F)
			return;
		vehicle->var_1F = al;
		invalidate_sprite_2((rct_sprite*)vehicle);
		return;
	}

	vehicle->status = VEHICLE_STATUS_ARRIVING;
	vehicle_invalidate_window(vehicle);
	vehicle->sub_state = 0;
	vehicle->var_C0 = 0;
}

/* rct2: 0x006D92FF */
static void vehicle_update_rotating(rct_vehicle* vehicle) {
	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0)
		return;

	rct_ride* ride = GET_RIDE(vehicle->ride);
	rct_ride_type* rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);

	uint8* edi;
	if (rideEntry->flags & RIDE_ENTRY_FLAG_ALTERNATIVE_ROTATION_MODE_1) {
		edi = RCT2_ADDRESS(0x0099F0F4, uint8*)[vehicle->sub_state];
	}
	else if (rideEntry->flags & RIDE_ENTRY_FLAG_ALTERNATIVE_ROTATION_MODE_2) {
		edi = RCT2_ADDRESS(0x009A2428, uint8*)[vehicle->sub_state];
	}
	else {
		edi = RCT2_ADDRESS(0x0099EB1C, uint8*)[vehicle->sub_state];
	}

	sint32 var_4C = (sint16)vehicle->var_4C;
	if (RCT2_GLOBAL(0x00F64E34, uint8) == BREAKDOWN_CONTROL_FAILURE) {
		var_4C += (ride->var_1AC >> 6) + 1;
	}
	var_4C++;

	uint8 al = edi[(uint32)var_4C];
	if (al != 0xFF) {
		vehicle->var_4C = (uint16)var_4C;
		if (al == vehicle->var_1F)
			return;
		vehicle->var_1F = al;
		invalidate_sprite_2((rct_sprite*)vehicle);
		return;
	}

	vehicle->var_4C = 0xFFFF;
	vehicle->var_CE++;
	if (RCT2_GLOBAL(0x00F64E34, uint8) != BREAKDOWN_CONTROL_FAILURE) {
		bool shouldStop = true;
		if (ride->status != RIDE_STATUS_CLOSED) {
			al = vehicle->var_CE + 1;
			if (ride->type == RIDE_TYPE_ENTERPRISE)
				al += 9;

			if (al < ride->rotations)
				shouldStop = false;
		}

		if (shouldStop == true) {
			if (vehicle->sub_state == 2) {
				vehicle->status = VEHICLE_STATUS_ARRIVING;
				vehicle_invalidate_window(vehicle);
				vehicle->sub_state = 0;
				vehicle->var_C0 = 0;
				return;
			}
			vehicle->sub_state++;
			vehicle_update_rotating(vehicle);
			return;
		}
	}

	if (ride->type == RIDE_TYPE_ENTERPRISE &&
		vehicle->sub_state == 2) {
		vehicle->status = VEHICLE_STATUS_ARRIVING;
		vehicle_invalidate_window(vehicle);
		vehicle->sub_state = 0;
		vehicle->var_C0 = 0;
		return;
	}

	vehicle->sub_state = 1;
	vehicle_update_rotating(vehicle);
}

/* rct2: 0x006D97CB */
static void vehicle_update_space_rings_operating(rct_vehicle* vehicle) {
	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0)
		return;

	uint8* edi = RCT2_ADDRESS(0x009A0ACC, uint8*)[vehicle->sub_state];

	uint8 al = edi[(uint16)(vehicle->var_4C + 1)];
	if (al != 0xFF) {
		vehicle->var_4C++;
		if (al == vehicle->var_1F)
			return;
		vehicle->var_1F = al;
		invalidate_sprite_2((rct_sprite*)vehicle);
		return;
	}

	vehicle->status = VEHICLE_STATUS_ARRIVING;
	vehicle_invalidate_window(vehicle);
	vehicle->sub_state = 0;
	vehicle->var_C0 = 0;
}

/* rct2: 0x006D9641*/
static void vehicle_update_haunted_house_operating(rct_vehicle* vehicle) {
	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0)
		return;

	if (vehicle->var_1F != 0) {
		if (RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_TICKS, uint32) & 1) {
			vehicle->var_1F++;
			invalidate_sprite_2((rct_sprite*)vehicle);

			if (vehicle->var_1F == 19)
				vehicle->var_1F = 0;
		}
	}

	uint16* edi = RCT2_ADDRESS(0x009A0ABC, uint16*)[vehicle->sub_state];

	uint16 ax = *edi;
	if ((uint16)(vehicle->var_4C + 1) > ax) {
		vehicle->status = VEHICLE_STATUS_ARRIVING;
		vehicle_invalidate_window(vehicle);
		vehicle->sub_state = 0;
		vehicle->var_C0 = 0;
		return;
	}

	vehicle->var_4C++;
	switch (vehicle->var_4C) {
	case 45:
		audio_play_sound_at_location(
			SOUND_HAUNTED_HOUSE_SCARE,
			vehicle->x,
			vehicle->y,
			vehicle->z);
		break;
	case 75:
		vehicle->var_1F = 1;
		invalidate_sprite_2((rct_sprite*)vehicle);
		break;
	case 400:
		audio_play_sound_at_location(
			SOUND_HAUNTED_HOUSE_SCREAM_1,
			vehicle->x,
			vehicle->y,
			vehicle->z);
		break;
	case 745:
		audio_play_sound_at_location(
			SOUND_HAUNTED_HOUSE_SCARE,
			vehicle->x,
			vehicle->y,
			vehicle->z);
		break;
	case 775:
		vehicle->var_1F = 1;
		invalidate_sprite_2((rct_sprite*)vehicle);
		break;
	case 1100:
		audio_play_sound_at_location(
			SOUND_HAUNTED_HOUSE_SCREAM_2,
			vehicle->x,
			vehicle->y,
			vehicle->z);
		break;
	}
}

/* rct2: 0x006d9781 */
static void vehicle_update_crooked_house_operating(rct_vehicle* vehicle) {
	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0)
		return;

	if ((uint16)(vehicle->var_4C + 1) >  RideCrookedHouseLength[vehicle->sub_state]) {
		vehicle->status = VEHICLE_STATUS_ARRIVING;
		vehicle_invalidate_window(vehicle);
		vehicle->sub_state = 0;
		vehicle->var_C0 = 0;
		return;
	}

	vehicle->var_4C++;
}

/* rct2: 0x006D9547 */
static void vehicle_update_top_spin_operating(rct_vehicle* vehicle) {
	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0)
		return;

	uint8* edi = RCT2_ADDRESS(0x009A12E0, uint8*)[vehicle->sub_state];

	uint8 al = edi[((sint16)vehicle->var_4C + 1) * 2];
	if (al != 0xFF) {
		vehicle->var_4C = (sint16)vehicle->var_4C + 1;
		if (al != vehicle->var_1F) {
			vehicle->var_1F = al;
			invalidate_sprite_2((rct_sprite*)vehicle);
		}
		al = edi[vehicle->var_4C * 2 + 1];
		if (al != vehicle->var_20) {
			vehicle->var_20 = al;
			invalidate_sprite_2((rct_sprite*)vehicle);
		}
		return;
	}

	vehicle->status = VEHICLE_STATUS_ARRIVING;
	vehicle_invalidate_window(vehicle);
	vehicle->sub_state = 0;
	vehicle->var_C0 = 0;
}

/**
*
*  rct2: 0x006D9CE9
*/
static void vehicle_update_waiting_for_cable_lift(rct_vehicle *vehicle)
{
	rct_ride* ride = GET_RIDE(vehicle->ride);

	rct_vehicle* cableLift = GET_VEHICLE(ride->cable_lift);

	if (cableLift->status != VEHICLE_STATUS_WAITING_FOR_PASSENGERS)
		return;

	cableLift->status = VEHICLE_STATUS_WAITING_TO_DEPART;
	cableLift->var_C0 = vehicle->sprite_index;
}

/**
*
*  rct2: 0x006D95AD
*/
static void vehicle_update_showing_film(rct_vehicle *vehicle)
{
	int currentTime, totalTime;

	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0)
		return;

	totalTime = RideFilmLength[vehicle->sub_state];
	currentTime = vehicle->var_4C + 1;
	if (currentTime <= totalTime) {
		vehicle->var_4C = currentTime;
	}
	else {
		vehicle->status = VEHICLE_STATUS_ARRIVING;
		vehicle_invalidate_window(vehicle);
		vehicle->sub_state = 0;
		vehicle->var_C0 = 0;
	}
}

/**
 *
 *  rct2: 0x006D95F7
 */
static void vehicle_update_doing_circus_show(rct_vehicle *vehicle)
{
	int currentTime, totalTime;

	if (RCT2_GLOBAL(0x00F64E34, uint8) == 0)
		return;

	totalTime = *(RCT2_ADDRESS(0x009A0AB4, uint16*)[vehicle->sub_state]);
	currentTime = vehicle->var_4C + 1;
	if (currentTime <= totalTime) {
		vehicle->var_4C = currentTime;
	} else {
		vehicle->status = VEHICLE_STATUS_ARRIVING;
		vehicle_invalidate_window(vehicle);
		vehicle->sub_state = 0;
		vehicle->var_C0 = 0;
	}
}

/* rct2: 0x0068B8BD
 * returns the map element that the vehicle will collide with
 * returns NULL if no collisions.
 */
static rct_map_element* vehicle_check_collision(sint16 x, sint16 y, sint16 z) {
	rct_map_element* mapElement = map_get_first_element_at(x / 32, y / 32);

	uint8 bl;
	if ((x & 0x1F) >= 16) {
		bl = 1;
		if ((y & 0x1F) < 16)
			bl = 2;
	}
	else {
		bl = 4;
		if ((y & 0x1F) >= 16)
			bl = 8;
	}

	do {
		if (z / 8 < mapElement->base_height)
			continue;

		if (z / 8 >= mapElement->clearance_height)
			continue;

		if (mapElement->flags & bl)
			return mapElement;
	} while (!map_element_is_last_for_tile(mapElement++));

	return NULL;
}

/* rct2: 0x006DE6C6 */
static void vehicle_kill_all_passengers(rct_vehicle* vehicle) {
	uint16 numFatalities = 0;

	uint16 spriteId = vehicle->sprite_index;
	for (rct_vehicle* curVehicle; spriteId != 0xFFFF; spriteId = curVehicle->next_vehicle_on_train) {
		curVehicle = GET_VEHICLE(spriteId);
		numFatalities += curVehicle->num_peeps;
	}

	rct_ride* ride = GET_RIDE(vehicle->ride);
	RCT2_GLOBAL(RCT2_ADDRESS_COMMON_FORMAT_ARGS, uint16) = numFatalities;

	uint8 crashType = numFatalities == 0 ?
		RIDE_CRASH_TYPE_NO_FATALITIES :
		RIDE_CRASH_TYPE_FATALITIES;

	if (crashType >= ride->last_crash_type)
		ride->last_crash_type = crashType;

	if (numFatalities != 0) {
		if (!(RCT2_GLOBAL(RCT2_ADDRESS_PARK_FLAGS, uint32) & PARK_FLAGS_SIX_FLAGS_DEPRECATED)) {
			RCT2_GLOBAL(RCT2_ADDRESS_COMMON_FORMAT_ARGS + 2, uint16) = ride->name;
			RCT2_GLOBAL(RCT2_ADDRESS_COMMON_FORMAT_ARGS + 4, uint32) = ride->name_arguments;

			news_item_add_to_queue(NEWS_ITEM_RIDE, 2219, vehicle->ride);
		}

		if (RCT2_GLOBAL(0x135882E, uint16) < 500) {
			RCT2_GLOBAL(0x135882E, uint16) += 200;
		}
	}

	spriteId = vehicle->sprite_index;
	for (rct_vehicle* curVehicle; spriteId != 0xFFFF; spriteId = curVehicle->next_vehicle_on_train) {
		curVehicle = GET_VEHICLE(spriteId);

		if (curVehicle->num_peeps != curVehicle->next_free_seat)
			continue;

		if (curVehicle->num_peeps == 0)
			continue;

		for (uint8 i = 0; i < curVehicle->num_peeps; i++) {
			rct_peep* peep = GET_PEEP(curVehicle->peep[i]);
			if (peep->outside_of_park == 0) {
				RCT2_GLOBAL(RCT2_ADDRESS_GUESTS_IN_PARK, uint16)--;
				RCT2_GLOBAL(RCT2_ADDRESS_BTM_TOOLBAR_DIRTY_FLAGS, uint16) |=
					BTM_TB_DIRTY_FLAG_PEEP_COUNT;
			}
			ride->num_riders--;
			peep_sprite_remove(peep);
		}

		curVehicle->num_peeps = 0;
		curVehicle->next_free_seat = 0;
	}
}

static void vehicle_crash_on_land(rct_vehicle* vehicle) {
	vehicle->status = VEHICLE_STATUS_CRASHED;
	vehicle_invalidate_window(vehicle);

	rct_ride* ride = GET_RIDE(vehicle->ride);
	if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_CRASHED)) {

		rct_vehicle* frontVehicle = vehicle;
		while (frontVehicle->is_child != 0)frontVehicle = GET_VEHICLE(frontVehicle->prev_vehicle_on_ride);

		uint8 trainIndex = 0;
		while (ride->vehicles[trainIndex] != frontVehicle->sprite_index)
			trainIndex++;

		ride_crash(vehicle->ride, trainIndex);

		if (ride->status != RIDE_STATUS_CLOSED) {
			ride_set_status(vehicle->ride, RIDE_STATUS_CLOSED);
		}
	}
	ride->lifecycle_flags |= RIDE_LIFECYCLE_CRASHED;
	ride->window_invalidate_flags |= RIDE_INVALIDATE_RIDE_MAIN | RIDE_INVALIDATE_RIDE_LIST;

	if (vehicle->is_child == 0) {
		vehicle_kill_all_passengers(vehicle);
	}

	vehicle->sub_state = 2;
	audio_play_sound_at_location(SOUND_CRASH, vehicle->x, vehicle->y, vehicle->z);

	sprite_misc_3_create(vehicle->x, vehicle->y, vehicle->z);
	sprite_misc_5_create(vehicle->x, vehicle->y, vehicle->z);

	uint8 numParticles = min(vehicle->sprite_width, 7);

	while (numParticles-- != 0)
		crashed_vehicle_particle_create(vehicle->colours, vehicle->x, vehicle->y, vehicle->z);

	vehicle->var_0C |= (1 << 7);
	vehicle->var_C5 = 0;
	vehicle->var_C8 = 0;
	vehicle->sprite_width = 13;
	vehicle->sprite_height_negative = 45;
	vehicle->sprite_height_positive = 5;

	sprite_move(vehicle->x, vehicle->y, vehicle->z, (rct_sprite*)vehicle);
	invalidate_sprite_2((rct_sprite*)vehicle);

	vehicle->var_4E = 0;
}

static void vehicle_crash_on_water(rct_vehicle* vehicle) {
	vehicle->status = VEHICLE_STATUS_CRASHED;
	vehicle_invalidate_window(vehicle);

	rct_ride* ride = GET_RIDE(vehicle->ride);
	if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_CRASHED)) {

		rct_vehicle* frontVehicle = vehicle;
		while (frontVehicle->is_child != 0)frontVehicle = GET_VEHICLE(frontVehicle->prev_vehicle_on_ride);

		uint8 trainIndex = 0;
		while (ride->vehicles[trainIndex] != frontVehicle->sprite_index)
			trainIndex++;

		ride_crash(vehicle->ride, trainIndex);

		if (ride->status != RIDE_STATUS_CLOSED) {
			ride_set_status(vehicle->ride, RIDE_STATUS_CLOSED);
		}
	}
	ride->lifecycle_flags |= RIDE_LIFECYCLE_CRASHED;
	ride->window_invalidate_flags |= RIDE_INVALIDATE_RIDE_MAIN | RIDE_INVALIDATE_RIDE_LIST;

	if (vehicle->is_child == 0) {
		vehicle_kill_all_passengers(vehicle);
	}

	vehicle->sub_state = 2;
	audio_play_sound_at_location(SOUND_WATER_1, vehicle->x, vehicle->y, vehicle->z);

	crash_splash_create(vehicle->x, vehicle->y, vehicle->z);
	crash_splash_create(vehicle->x - 8, vehicle->y - 9, vehicle->z);
	crash_splash_create(vehicle->x + 11, vehicle->y - 9, vehicle->z);
	crash_splash_create(vehicle->x + 11, vehicle->y + 8, vehicle->z);
	crash_splash_create(vehicle->x - 4, vehicle->y + 8, vehicle->z);

	for (int i = 0; i < 10; ++i)
		crashed_vehicle_particle_create(vehicle->colours, vehicle->x - 4, vehicle->y + 8, vehicle->z);

	vehicle->var_0C |= (1 << 7);
	vehicle->var_C5 = 0;
	vehicle->var_C8 = 0;
	vehicle->sprite_width = 13;
	vehicle->sprite_height_negative = 45;
	vehicle->sprite_height_positive = 5;

	sprite_move(vehicle->x, vehicle->y, vehicle->z, (rct_sprite*)vehicle);
	invalidate_sprite_2((rct_sprite*)vehicle);

	vehicle->var_4E = 0xFFFF;
}

/**
*
*  rct2: 0x006D9D21
*/
static void vehicle_update_travelling_cable_lift(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);

	if (vehicle->sub_state == 0) {
		if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_BROKEN_TRAIN) {
			if (ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN)
				return;

			ride->lifecycle_flags |= RIDE_LIFECYCLE_BROKEN_DOWN;
			ride_breakdown_add_news_item(vehicle->ride);
			ride->window_invalidate_flags |=
				RIDE_INVALIDATE_RIDE_MAIN |
				RIDE_INVALIDATE_RIDE_LIST |
				RIDE_INVALIDATE_RIDE_MAINTENANCE;

			ride->mechanic_status = RIDE_MECHANIC_STATUS_CALLING;
			ride->inspection_station = vehicle->current_station;
			ride->breakdown_reason = ride->breakdown_reason_pending;
			vehicle->velocity = 0;
			return;
		}

		vehicle->sub_state = 1;
		vehicle_peep_easteregg_here_we_are(vehicle);
		if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_TESTED)) {
			if (!(vehicle->update_flags & VEHICLE_UPDATE_FLAG_TESTING)) {
				if (!(ride->lifecycle_flags & RIDE_LIFECYCLE_TEST_IN_PROGRESS)) {
					vehicle_test_reset(vehicle);
				}
				else {
					uint8 currentSegment = ride->current_test_segment + 1;
					if (currentSegment >= ride->num_stations) {
						vehicle_update_test_finish(vehicle);
					}
					else {
						ride->current_test_segment = currentSegment;
						ride->var_1F6 = vehicle->current_station;
					}
				}
			}
		}
	}

	rct_ride_type* rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);
	rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

	if (vehicle->velocity <= 439800) {
		vehicle->var_2C = 4398;
	}
	int flags = vehicle_update_track_motion(vehicle, NULL);

	if (flags & (1 << 11)) {
		vehicle->status = VEHICLE_STATUS_TRAVELLING;
		vehicle_invalidate_window(vehicle);
		vehicle->sub_state = 1;
		vehicle->lost_time_out = 0;
		return;
	}

	if (vehicle->sub_state == 2)
		return;

	if (flags & (1 << 3) && vehicle->current_station == RCT2_GLOBAL(0x00F64E1C, uint8))
		return;

	vehicle->sub_state = 2;

	if (ride->mode == RIDE_MODE_CONTINUOUS_CIRCUIT_BLOCK_SECTIONED ||
		ride->mode == RIDE_MODE_POWERED_LAUNCH_BLOCK_SECTIONED)
		return;

	// This is slightly different to the vanilla function
	ride->station_depart[vehicle->current_station] &= STATION_DEPART_FLAG;
	uint8 waitingTime = 3;
	if (ride->depart_flags & RIDE_DEPART_WAIT_FOR_MINIMUM_LENGTH) {
		waitingTime = max(ride->min_waiting_time, 3);
		waitingTime = min(waitingTime, 127);
	}

	ride->station_depart[vehicle->current_station] |= waitingTime;
}

/**
*
*  rct2: 0x006D98CA
*/
static void vehicle_update_crash(rct_vehicle *vehicle){
	uint16 spriteId = vehicle->sprite_index;
	rct_vehicle* curVehicle;
	do {
		curVehicle = GET_VEHICLE(spriteId);
		if (curVehicle->sub_state > 1) {
			if (curVehicle->var_4E <= 96) {
				curVehicle->var_4E++;
				if ((scenario_rand() & 0xFFFF) <= 0x1555) {
					sprite_misc_3_create(
						curVehicle->x + (scenario_rand() & 2 - 1),
						curVehicle->y + (scenario_rand() & 2 - 1),
						curVehicle->z
						);
				}
			}
			if (curVehicle->var_C8 + 7281 > 0xFFFF) {
				curVehicle->var_C5++;
				if (curVehicle->var_C5 >= 8)
					curVehicle->var_C5 = 0;
				invalidate_sprite_2((rct_sprite*)curVehicle);
			}
			curVehicle->var_C8 += 7281;
			continue;
		}

		rct_map_element* collideElement = vehicle_check_collision(curVehicle->x, curVehicle->y, curVehicle->z);
		if (collideElement == NULL) {
			curVehicle->sub_state = 1;
		}
		else if (curVehicle->sub_state == 1){
			vehicle_crash_on_land(curVehicle);
			continue;
		}

		int z = map_element_height(curVehicle->x, curVehicle->y);
		sint16 waterHeight = (z >> 16) & 0xFFFF;
		z = (sint16)(z & 0xFFFF);
		sint16 zDiff;
		if (waterHeight != 0) {
			zDiff = curVehicle->z - waterHeight;
			if (zDiff <= 0 && zDiff >= -20) {
				vehicle_crash_on_water(curVehicle);
				continue;
			}
		}

		zDiff = curVehicle->z - z;
		if ((zDiff <= 0 && zDiff >= -20) || curVehicle->z < 16){
			vehicle_crash_on_land(curVehicle);
			continue;
		}

		invalidate_sprite_2((rct_sprite*)curVehicle);

		rct_xyz16 curPosition = {
			.x = curVehicle->x,
			.y = curVehicle->y,
			.z = curVehicle->z
		};

		curPosition.x += (sint8)(curVehicle->var_B6 >> 8);
		curPosition.y += (sint8)(curVehicle->var_C0 >> 8);
		curPosition.z += (sint8)(curVehicle->var_4E >> 8);
		curVehicle->track_x += (sint16)(curVehicle->var_B6 << 8);
		curVehicle->track_y += (sint16)(curVehicle->var_C0 << 8);
		curVehicle->track_z += (sint16)(curVehicle->var_4E << 8);

		if (curPosition.x > 0x1FFF ||
			curPosition.y > 0x1FFF) {
			vehicle_crash_on_land(curVehicle);
			continue;
		}

		sprite_move(curPosition.x, curPosition.y, curPosition.z, (rct_sprite*)curVehicle);
		invalidate_sprite_2((rct_sprite*)curVehicle);

		if (curVehicle->sub_state == 1) {
			curVehicle->var_4E += 0xFFEC;
		}
	} while ((spriteId = curVehicle->next_vehicle_on_train) != 0xFFFF);
}
/**
 *
 *  rct2: 0x006D7888
 */
static void vehicle_update_sound(rct_vehicle *vehicle)
{
	rct_ride *ride;
	rct_ride_type *rideEntry;
	// bl should be set before hand
	uint8 bl = 255, dl = 255;
	// bh screamVolume should be set before hand
	uint8 screamId, screamVolume = 255;
	uint16 soundIdVolume;

	ride = GET_RIDE(vehicle->ride);
	rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);

	rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

	int ecx = abs(vehicle->velocity) - 0x10000;
	if (ecx >= 0) {
		dl = vehicleEntry->var_57;
		ecx >>= 15;
		bl = min(208 + ecx & 0xFF, 255);
	}

	switch (vehicleEntry->sound_range) {
	case 3:
		screamId = vehicle->scream_sound_id;
		if (!(RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_TICKS, uint32) & 0x7F)) {
			if (vehicle->velocity < 0x40000 || vehicle->scream_sound_id != 255)
				goto loc_6D7A97;

			if ((scenario_rand() & 0xFFFF) <= 0x5555) {
				vehicle->scream_sound_id = SOUND_TRAIN_WHISTLE;
				screamVolume = 255;
				break;
			}
		}
		if (screamId == NO_SCREAM) screamId = 255;
		screamVolume = 255;
		break;

	case 4:
		screamId = vehicle->scream_sound_id;
		if (!(RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_TICKS, uint32) & 0x7F)) {
			if (vehicle->velocity < 0x40000 || vehicle->scream_sound_id != 255)
				goto loc_6D7A97;

			if ((scenario_rand() & 0xFFFF) <= 0x5555) {
				vehicle->scream_sound_id = SOUND_TRAM;
				screamVolume = 255;
				break;
			}
		}
		if (screamId == NO_SCREAM) screamId = 255;
		screamVolume = 255;
		break;

	default:
		if ((vehicleEntry->var_14 & 0x10)) {
			screamId = vehicle_update_scream_sound(vehicle);
			if (screamId == NO_SCREAM)
				screamId = 255;
			if (screamId == 255)
				goto loc_6D7A97;
			break;
		}

	loc_6D7A97:
		vehicle->scream_sound_id = 255;
		assert(ride->type < countof(RideLiftData));
		// Get lift hill sound
		screamId = RideLiftData[ride->type].sound_id;
		screamVolume = 243;
		if (!(vehicle->var_B8 & 2))
			screamId = 255;
	}

	// Friction sound
	soundIdVolume = sub_6D7AC0(vehicle->sound1_id, vehicle->sound1_volume, dl, bl);
	vehicle->sound1_id = soundIdVolume & 0xFF;
	vehicle->sound1_volume = (soundIdVolume >> 8) & 0xFF;

	// Scream sound
	soundIdVolume = sub_6D7AC0(vehicle->sound2_id, vehicle->sound2_volume, screamId, screamVolume);
	vehicle->sound2_id = soundIdVolume & 0xFF;
	vehicle->sound2_volume = (soundIdVolume >> 8) & 0xFF;

	{
		int ebx = RCT2_ADDRESS(0x009A3684, sint16)[vehicle->sprite_direction];
		int eax = ((vehicle->velocity >> 14) * ebx) >> 14;
		eax = clamp(-127, eax, 127);

		vehicle->var_BF = eax & 0xFF;
	}
}

/**
 *
 *
 *  rct2: 0x006D796B
 */
static int vehicle_update_scream_sound(rct_vehicle *vehicle)
{
	uint32 r;
	uint16 spriteIndex;
	rct_ride_type *rideEntry;
	rct_vehicle *vehicle2;

	rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);

	rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

	int totalNumPeeps = vehicle_get_total_num_peeps(vehicle);
	if (totalNumPeeps == 0)
		return 255;

	if (vehicle->velocity < 0) {
		if (vehicle->velocity > -0x2C000)
			return 255;

		spriteIndex = vehicle->sprite_index;
		do {
			vehicle2 = &(g_sprite_list[spriteIndex].vehicle);
			if (vehicle2->var_1F < 1)
				continue;
			if (vehicle2->var_1F <= 4)
				goto produceScream;
			if (vehicle2->var_1F < 9)
				continue;
			if (vehicle2->var_1F <= 15)
				goto produceScream;
		} while ((spriteIndex = vehicle2->next_vehicle_on_train) != SPRITE_INDEX_NULL);
		return 255;
	}

	if (vehicle->velocity < 0x2C000)
		return 255;

	spriteIndex = vehicle->sprite_index;
	do {
		vehicle2 = &(g_sprite_list[spriteIndex].vehicle);
		if (vehicle2->var_1F < 5)
			continue;
		if (vehicle2->var_1F <= 8)
			goto produceScream;
		if (vehicle2->var_1F < 17)
			continue;
		if (vehicle2->var_1F <= 23)
			goto produceScream;
	} while ((spriteIndex = vehicle2->next_vehicle_on_train) != SPRITE_INDEX_NULL);
	return 255;

produceScream:
	if (vehicle->scream_sound_id == 255) {
		r = scenario_rand();
		if (totalNumPeeps >= (int)(r % 16)) {
			switch (vehicleEntry->sound_range) {
			case 0:
				vehicle->scream_sound_id = byte_9A3A14[r % 2];
				break;
			case 1:
				vehicle->scream_sound_id = byte_9A3A18[r % 7];
				break;
			case 2:
				vehicle->scream_sound_id = byte_9A3A16[r % 2];
				break;
			default:
				vehicle->scream_sound_id = NO_SCREAM;
				break;
			}
		} else {
			vehicle->scream_sound_id = NO_SCREAM;
		}
	}
	return vehicle->scream_sound_id;
}

/**
 *
 *  rct2: 0x006D73D0
 * ax: verticalG
 * dx: lateralG
 * esi: vehicle
 */
void vehicle_get_g_forces(rct_vehicle *vehicle, int *verticalG, int *lateralG)
{
	int gForceVert = (((sint64)0x280000) * RCT2_ADDRESS(0x009A37E4, sint32)[vehicle->var_1F]) >> 32;
	gForceVert = (((sint64)gForceVert) * RCT2_ADDRESS(0x009A39C4, sint32)[vehicle->var_20]) >> 32;
	int lateralFactor = 0, vertFactor = 0;

	// Note shr has meant some of the below functions cast a known negative number to 
	// unsigned. Possibly an original bug but will be left implemented.
	switch (vehicle->track_type >> 2) {
	case TRACK_ELEM_FLAT:
	case TRACK_ELEM_END_STATION:
	case TRACK_ELEM_BEGIN_STATION:
	case TRACK_ELEM_MIDDLE_STATION:
	case TRACK_ELEM_25_DEG_UP:
	case TRACK_ELEM_60_DEG_UP:	//
	case TRACK_ELEM_25_DEG_DOWN:
	case TRACK_ELEM_60_DEG_DOWN: //
	case TRACK_ELEM_FLAT_TO_LEFT_BANK:
	case TRACK_ELEM_FLAT_TO_RIGHT_BANK:
	case TRACK_ELEM_LEFT_BANK_TO_FLAT:
	case TRACK_ELEM_RIGHT_BANK_TO_FLAT://
	case TRACK_ELEM_LEFT_BANK:
	case TRACK_ELEM_RIGHT_BANK:
	case TRACK_ELEM_TOWER_BASE:
	case TRACK_ELEM_TOWER_SECTION:
	case TRACK_ELEM_FLAT_COVERED:
	case TRACK_ELEM_25_DEG_UP_COVERED:
	case TRACK_ELEM_60_DEG_UP_COVERED:
	case TRACK_ELEM_25_DEG_DOWN_COVERED:
	case TRACK_ELEM_60_DEG_DOWN_COVERED:
	case TRACK_ELEM_BRAKES:
	case TRACK_ELEM_ROTATION_CONTROL_TOGGLE:
	case TRACK_ELEM_INVERTED_90_DEG_UP_TO_FLAT_QUARTER_LOOP:
	case TRACK_ELEM_25_DEG_UP_LEFT_BANKED:
	case TRACK_ELEM_25_DEG_UP_RIGHT_BANKED:
	case TRACK_ELEM_WATERFALL:
	case TRACK_ELEM_RAPIDS:
	case TRACK_ELEM_ON_RIDE_PHOTO:
	case TRACK_ELEM_25_DEG_DOWN_LEFT_BANKED:
	case TRACK_ELEM_25_DEG_DOWN_RIGHT_BANKED:
	case TRACK_ELEM_WHIRLPOOL:
	case TRACK_ELEM_REVERSE_WHOA_BELLY_VERTICAL:
	case TRACK_ELEM_90_DEG_UP:
	case TRACK_ELEM_90_DEG_DOWN:
	case TRACK_ELEM_DIAG_FLAT:
	case TRACK_ELEM_DIAG_25_DEG_UP:
	case TRACK_ELEM_DIAG_60_DEG_UP:
	case TRACK_ELEM_DIAG_25_DEG_DOWN:
	case TRACK_ELEM_DIAG_60_DEG_DOWN:
	case TRACK_ELEM_DIAG_FLAT_TO_LEFT_BANK:
	case TRACK_ELEM_DIAG_FLAT_TO_RIGHT_BANK:
	case TRACK_ELEM_DIAG_LEFT_BANK_TO_FLAT:
	case TRACK_ELEM_DIAG_RIGHT_BANK_TO_FLAT:
	case TRACK_ELEM_DIAG_LEFT_BANK:
	case TRACK_ELEM_DIAG_RIGHT_BANK:
	case TRACK_ELEM_LOG_FLUME_REVERSER:
	case TRACK_ELEM_SPINNING_TUNNEL:
	case TRACK_ELEM_POWERED_LIFT:
	case TRACK_ELEM_MINI_GOLF_HOLE_A:
	case TRACK_ELEM_MINI_GOLF_HOLE_B:
	case TRACK_ELEM_MINI_GOLF_HOLE_C:
	case TRACK_ELEM_MINI_GOLF_HOLE_D:
	case TRACK_ELEM_MINI_GOLF_HOLE_E:
	case TRACK_ELEM_LEFT_REVERSER:
	case TRACK_ELEM_RIGHT_REVERSER:
	case TRACK_ELEM_AIR_THRUST_VERTICAL_DOWN:
	case TRACK_ELEM_BLOCK_BRAKES:
	case TRACK_ELEM_25_DEG_UP_TO_LEFT_BANKED_25_DEG_UP:
	case TRACK_ELEM_25_DEG_UP_TO_RIGHT_BANKED_25_DEG_UP:
	case TRACK_ELEM_LEFT_BANKED_25_DEG_UP_TO_25_DEG_UP:
	case TRACK_ELEM_RIGHT_BANKED_25_DEG_UP_TO_25_DEG_UP:
	case TRACK_ELEM_25_DEG_DOWN_TO_LEFT_BANKED_25_DEG_DOWN:
	case TRACK_ELEM_25_DEG_DOWN_TO_RIGHT_BANKED_25_DEG_DOWN:
	case TRACK_ELEM_LEFT_BANKED_25_DEG_DOWN_TO_25_DEG_DOWN:
	case TRACK_ELEM_RIGHT_BANKED_25_DEG_DOWN_TO_25_DEG_DOWN:
	case TRACK_ELEM_LEFT_QUARTER_TURN_1_TILE_90_DEG_UP:
	case TRACK_ELEM_RIGHT_QUARTER_TURN_1_TILE_90_DEG_UP:
	case TRACK_ELEM_LEFT_QUARTER_TURN_1_TILE_90_DEG_DOWN:
	case TRACK_ELEM_RIGHT_QUARTER_TURN_1_TILE_90_DEG_DOWN:
		//6d73FF
		// Do nothing
		break;
	case TRACK_ELEM_FLAT_TO_25_DEG_UP://
	case TRACK_ELEM_25_DEG_DOWN_TO_FLAT://
	case TRACK_ELEM_LEFT_BANK_TO_25_DEG_UP:
	case TRACK_ELEM_RIGHT_BANK_TO_25_DEG_UP:
	case TRACK_ELEM_25_DEG_DOWN_TO_LEFT_BANK:
	case TRACK_ELEM_25_DEG_DOWN_TO_RIGHT_BANK:
	case TRACK_ELEM_FLAT_TO_25_DEG_UP_COVERED:
	case TRACK_ELEM_25_DEG_DOWN_TO_FLAT_COVERED:
	case TRACK_ELEM_LEFT_BANKED_FLAT_TO_LEFT_BANKED_25_DEG_UP:
	case TRACK_ELEM_RIGHT_BANKED_FLAT_TO_RIGHT_BANKED_25_DEG_UP:
	case TRACK_ELEM_LEFT_BANKED_25_DEG_DOWN_TO_LEFT_BANKED_FLAT:
	case TRACK_ELEM_RIGHT_BANKED_25_DEG_DOWN_TO_RIGHT_BANKED_FLAT:
	case TRACK_ELEM_FLAT_TO_LEFT_BANKED_25_DEG_UP:
	case TRACK_ELEM_FLAT_TO_RIGHT_BANKED_25_DEG_UP:
	case TRACK_ELEM_LEFT_BANKED_25_DEG_DOWN_TO_FLAT:
	case TRACK_ELEM_RIGHT_BANKED_25_DEG_DOWN_TO_FLAT:
		vertFactor = 103;
		//6d7509
		break;
	case TRACK_ELEM_25_DEG_UP_TO_FLAT://
	case TRACK_ELEM_FLAT_TO_25_DEG_DOWN://
	case TRACK_ELEM_25_DEG_UP_TO_LEFT_BANK:
	case TRACK_ELEM_25_DEG_UP_TO_RIGHT_BANK:
	case TRACK_ELEM_LEFT_BANK_TO_25_DEG_DOWN:
	case TRACK_ELEM_RIGHT_BANK_TO_25_DEG_DOWN:
	case TRACK_ELEM_25_DEG_UP_TO_FLAT_COVERED:
	case TRACK_ELEM_FLAT_TO_25_DEG_DOWN_COVERED:
	case TRACK_ELEM_CABLE_LIFT_HILL:
	case TRACK_ELEM_LEFT_BANKED_25_DEG_UP_TO_LEFT_BANKED_FLAT:
	case TRACK_ELEM_RIGHT_BANKED_25_DEG_UP_TO_RIGHT_BANKED_FLAT:
	case TRACK_ELEM_LEFT_BANKED_FLAT_TO_LEFT_BANKED_25_DEG_DOWN:
	case TRACK_ELEM_RIGHT_BANKED_FLAT_TO_RIGHT_BANKED_25_DEG_DOWN:
	case TRACK_ELEM_LEFT_BANKED_25_DEG_UP_TO_FLAT:
	case TRACK_ELEM_RIGHT_BANKED_25_DEG_UP_TO_FLAT:
	case TRACK_ELEM_FLAT_TO_LEFT_BANKED_25_DEG_DOWN:
	case TRACK_ELEM_FLAT_TO_RIGHT_BANKED_25_DEG_DOWN:
		vertFactor = -103;
		//6d7569
		break;
	case TRACK_ELEM_25_DEG_UP_TO_60_DEG_UP://
	case TRACK_ELEM_60_DEG_DOWN_TO_25_DEG_DOWN://
	case TRACK_ELEM_25_DEG_UP_TO_60_DEG_UP_COVERED:
	case TRACK_ELEM_60_DEG_DOWN_TO_25_DEG_DOWN_COVERED:
		vertFactor = 82;
		//6d7545
		break;
	case TRACK_ELEM_60_DEG_UP_TO_25_DEG_UP://
	case TRACK_ELEM_25_DEG_DOWN_TO_60_DEG_DOWN://
	case TRACK_ELEM_60_DEG_UP_TO_25_DEG_UP_COVERED:
	case TRACK_ELEM_25_DEG_DOWN_TO_60_DEG_DOWN_COVERED:
		vertFactor = -82;
		//6d7551
		break;
	case TRACK_ELEM_LEFT_QUARTER_TURN_5_TILES://
	case TRACK_ELEM_LEFT_QUARTER_TURN_5_TILES_25_DEG_UP:
	case TRACK_ELEM_LEFT_QUARTER_TURN_5_TILES_25_DEG_DOWN:
	case TRACK_ELEM_LEFT_TWIST_DOWN_TO_UP:
	case TRACK_ELEM_LEFT_TWIST_UP_TO_DOWN:
	case TRACK_ELEM_LEFT_QUARTER_TURN_5_TILES_COVERED:
	case TRACK_ELEM_LEFT_QUARTER_HELIX_LARGE_UP:
	case TRACK_ELEM_LEFT_QUARTER_HELIX_LARGE_DOWN:
	case TRACK_ELEM_LEFT_FLYER_TWIST_UP:
	case TRACK_ELEM_LEFT_FLYER_TWIST_DOWN:
	case TRACK_ELEM_LEFT_HEARTLINE_ROLL:
		lateralFactor = 98;
		//6d7590
		break;
	case TRACK_ELEM_RIGHT_QUARTER_TURN_5_TILES://
	case TRACK_ELEM_RIGHT_QUARTER_TURN_5_TILES_25_DEG_UP:
	case TRACK_ELEM_RIGHT_QUARTER_TURN_5_TILES_25_DEG_DOWN:
	case TRACK_ELEM_RIGHT_TWIST_DOWN_TO_UP:
	case TRACK_ELEM_RIGHT_TWIST_UP_TO_DOWN:
	case TRACK_ELEM_RIGHT_QUARTER_TURN_5_TILES_COVERED:
	case TRACK_ELEM_RIGHT_QUARTER_HELIX_LARGE_UP:
	case TRACK_ELEM_RIGHT_QUARTER_HELIX_LARGE_DOWN:
	case TRACK_ELEM_RIGHT_FLYER_TWIST_UP:
	case TRACK_ELEM_RIGHT_FLYER_TWIST_DOWN:
	case TRACK_ELEM_RIGHT_HEARTLINE_ROLL:
		lateralFactor = -98;
		//6d75B7
		break;
	case TRACK_ELEM_BANKED_LEFT_QUARTER_TURN_5_TILES:
	case TRACK_ELEM_LEFT_HALF_BANKED_HELIX_UP_LARGE:
	case TRACK_ELEM_LEFT_HALF_BANKED_HELIX_DOWN_LARGE:
	case TRACK_ELEM_LEFT_QUARTER_BANKED_HELIX_LARGE_UP:
	case TRACK_ELEM_LEFT_QUARTER_BANKED_HELIX_LARGE_DOWN:
		vertFactor = 200;
		lateralFactor = 160;
		//6d75E1
		break;
	case TRACK_ELEM_BANKED_RIGHT_QUARTER_TURN_5_TILES:
	case TRACK_ELEM_RIGHT_HALF_BANKED_HELIX_UP_LARGE:
	case TRACK_ELEM_RIGHT_HALF_BANKED_HELIX_DOWN_LARGE:
	case TRACK_ELEM_RIGHT_QUARTER_BANKED_HELIX_LARGE_UP:
	case TRACK_ELEM_RIGHT_QUARTER_BANKED_HELIX_LARGE_DOWN:
		vertFactor = 200;
		lateralFactor = -160;
		//6d75F0
		break;
	case TRACK_ELEM_S_BEND_LEFT:
	case TRACK_ELEM_S_BEND_LEFT_COVERED:
		lateralFactor = (vehicle->track_progress < 48) ? 98 : -98;
		//6d75FF
		break;
	case TRACK_ELEM_S_BEND_RIGHT:
	case TRACK_ELEM_S_BEND_RIGHT_COVERED:
		lateralFactor = (vehicle->track_progress < 48) ? -98 : 98;
		//6d7608
		break;
	case TRACK_ELEM_LEFT_VERTICAL_LOOP:
	case TRACK_ELEM_RIGHT_VERTICAL_LOOP:
		vertFactor = (abs(vehicle->track_progress - 155) / 2) + 28;
		//6d7690
		break;
	case TRACK_ELEM_LEFT_QUARTER_TURN_3_TILES:
	case TRACK_ELEM_LEFT_QUARTER_TURN_3_TILES_25_DEG_UP:
	case TRACK_ELEM_LEFT_QUARTER_TURN_3_TILES_25_DEG_DOWN:
	case TRACK_ELEM_LEFT_QUARTER_TURN_3_TILES_COVERED:
	case TRACK_ELEM_LEFT_CURVED_LIFT_HILL:
		lateralFactor = 59;
		//6d7704
		break;
	case TRACK_ELEM_RIGHT_QUARTER_TURN_3_TILES:
	case TRACK_ELEM_RIGHT_QUARTER_TURN_3_TILES_25_DEG_UP:
	case TRACK_ELEM_RIGHT_QUARTER_TURN_3_TILES_25_DEG_DOWN:
	case TRACK_ELEM_RIGHT_QUARTER_TURN_3_TILES_COVERED:
	case TRACK_ELEM_RIGHT_CURVED_LIFT_HILL:
		lateralFactor = -59;
		//6d7710
		break;
	case TRACK_ELEM_LEFT_QUARTER_TURN_3_TILES_BANK:
	case TRACK_ELEM_LEFT_HALF_BANKED_HELIX_UP_SMALL:
	case TRACK_ELEM_LEFT_HALF_BANKED_HELIX_DOWN_SMALL:
		vertFactor = 100;
		lateralFactor = 100;
		//6d7782
		break;
	case TRACK_ELEM_RIGHT_QUARTER_TURN_3_TILES_BANK:
	case TRACK_ELEM_RIGHT_HALF_BANKED_HELIX_UP_SMALL:
	case TRACK_ELEM_RIGHT_HALF_BANKED_HELIX_DOWN_SMALL:
		vertFactor = 100;
		lateralFactor = -100;
		//6d778E
		break;
	case TRACK_ELEM_LEFT_QUARTER_TURN_1_TILE:
		lateralFactor = 45;
		//6d779A
		break;
	case TRACK_ELEM_RIGHT_QUARTER_TURN_1_TILE:
		lateralFactor = -45;
		//6d77A3
		break;
	case TRACK_ELEM_HALF_LOOP_UP:
	case TRACK_ELEM_FLYER_HALF_LOOP_UP:
		vertFactor = (((uint16)(-(vehicle->track_progress - 155))) / 2) + 28;
		//6d763E
		break;
	case TRACK_ELEM_HALF_LOOP_DOWN:
	case TRACK_ELEM_FLYER_HALF_LOOP_DOWN:
		vertFactor = (vehicle->track_progress / 2) + 28;
		//6d7656
		break;
	case TRACK_ELEM_LEFT_CORKSCREW_UP:
	case TRACK_ELEM_RIGHT_CORKSCREW_DOWN:
	case TRACK_ELEM_LEFT_FLYER_CORKSCREW_UP:
	case TRACK_ELEM_RIGHT_FLYER_CORKSCREW_DOWN:
		vertFactor = 52;
		lateralFactor = 70;
		//6d76AA
		break;
	case TRACK_ELEM_RIGHT_CORKSCREW_UP:
	case TRACK_ELEM_LEFT_CORKSCREW_DOWN:
	case TRACK_ELEM_RIGHT_FLYER_CORKSCREW_UP:
	case TRACK_ELEM_LEFT_FLYER_CORKSCREW_DOWN:
		vertFactor = 52;
		lateralFactor = -70;
		//6d76B9
		break;
	case TRACK_ELEM_FLAT_TO_60_DEG_UP:
	case TRACK_ELEM_60_DEG_DOWN_TO_FLAT:
		vertFactor = 56;
		//6d747C
		break;
	case TRACK_ELEM_60_DEG_UP_TO_FLAT:
	case TRACK_ELEM_FLAT_TO_60_DEG_DOWN:
	case TRACK_ELEM_BRAKE_FOR_DROP:
		vertFactor = -56;
		//6d7488
		break;
	case TRACK_ELEM_LEFT_QUARTER_TURN_1_TILE_60_DEG_UP:
	case TRACK_ELEM_LEFT_QUARTER_TURN_1_TILE_60_DEG_DOWN:
		lateralFactor = 88;
		//6d7770
		break;
	case TRACK_ELEM_RIGHT_QUARTER_TURN_1_TILE_60_DEG_UP:
	case TRACK_ELEM_RIGHT_QUARTER_TURN_1_TILE_60_DEG_DOWN:
		lateralFactor = -88;
		//6d7779
		break;
	case TRACK_ELEM_WATER_SPLASH:
		vertFactor = -150;
		if (vehicle->track_progress < 32)
			break;
		vertFactor = 150;
		if (vehicle->track_progress < 64)
			break;
		vertFactor = 0;
		if (vehicle->track_progress < 96)
			break;
		vertFactor = 150;
		if (vehicle->track_progress < 128)
			break;
		vertFactor = -150;
		//6d7408
		break;
	case TRACK_ELEM_FLAT_TO_60_DEG_UP_LONG_BASE:
	case TRACK_ELEM_FLAT_TO_60_DEG_DOWN_LONG_BASE:
		vertFactor = 160;
		//6d74F1
		break;
	case TRACK_ELEM_60_DEG_UP_TO_FLAT_LONG_BASE:
	case TRACK_ELEM_60_DEG_UP_TO_FLAT_LONG_BASE_122:
		vertFactor = -160;
		//6d74FD
		break;
	case TRACK_ELEM_REVERSE_WHOA_BELLY_SLOPE:
	case TRACK_ELEM_AIR_THRUST_VERTICAL_DOWN_TO_LEVEL:
		vertFactor = 120;
		//6d7458
		break;
	case TRACK_ELEM_60_DEG_UP_TO_90_DEG_UP:
	case TRACK_ELEM_90_DEG_DOWN_TO_60_DEG_DOWN:
		vertFactor = 110;
		//6d7515
		break;
	case TRACK_ELEM_90_DEG_UP_TO_60_DEG_UP:
	case TRACK_ELEM_60_DEG_DOWN_TO_90_DEG_DOWN:
		vertFactor = -110;
		//6d7521
		break;
	case TRACK_ELEM_LEFT_EIGHTH_TO_DIAG:
	case TRACK_ELEM_LEFT_EIGHTH_TO_ORTHOGONAL:
		lateralFactor = 137;
		//6d7575
		break;
	case TRACK_ELEM_RIGHT_EIGHTH_TO_DIAG:
	case TRACK_ELEM_RIGHT_EIGHTH_TO_ORTHOGONAL:
		lateralFactor = -137;
		//6d759C
		break;
	case TRACK_ELEM_LEFT_EIGHTH_BANK_TO_DIAG:
	case TRACK_ELEM_LEFT_EIGHTH_BANK_TO_ORTHOGONAL:
		vertFactor = 270;
		lateralFactor = 200;
		//6d75C3
		break;
	case TRACK_ELEM_RIGHT_EIGHTH_BANK_TO_DIAG:
	case TRACK_ELEM_RIGHT_EIGHTH_BANK_TO_ORTHOGONAL:
		vertFactor = 270;
		lateralFactor = -200;
		//6d75D2
		break;
	case TRACK_ELEM_DIAG_FLAT_TO_25_DEG_UP:
	case TRACK_ELEM_DIAG_25_DEG_DOWN_TO_FLAT:
	case TRACK_ELEM_DIAG_LEFT_BANK_TO_25_DEG_UP:
	case TRACK_ELEM_DIAG_RIGHT_BANK_TO_25_DEG_UP:
	case TRACK_ELEM_DIAG_25_DEG_DOWN_TO_LEFT_BANK:
	case TRACK_ELEM_DIAG_25_DEG_DOWN_TO_RIGHT_BANK:
		vertFactor = 113;
		//6d7494
		break;
	case TRACK_ELEM_DIAG_25_DEG_UP_TO_FLAT:
	case TRACK_ELEM_DIAG_FLAT_TO_25_DEG_DOWN:
	case TRACK_ELEM_DIAG_25_DEG_UP_TO_LEFT_BANK:
	case TRACK_ELEM_DIAG_25_DEG_UP_TO_RIGHT_BANK:
	case TRACK_ELEM_DIAG_LEFT_BANK_TO_25_DEG_DOWN:
	case TRACK_ELEM_DIAG_RIGHT_BANK_TO_25_DEG_DOWN:
		vertFactor = -113;
		//6d755D
		break;	
	case TRACK_ELEM_DIAG_25_DEG_UP_TO_60_DEG_UP:
	case TRACK_ELEM_DIAG_60_DEG_DOWN_TO_25_DEG_DOWN:
		vertFactor = 95;
		//6D752D
		break;
	case TRACK_ELEM_DIAG_60_DEG_UP_TO_25_DEG_UP:
	case TRACK_ELEM_DIAG_25_DEG_DOWN_TO_60_DEG_DOWN:
		vertFactor = -95;
		//6D7539
		break;
	case TRACK_ELEM_DIAG_FLAT_TO_60_DEG_UP:
	case TRACK_ELEM_DIAG_60_DEG_DOWN_TO_FLAT:
		vertFactor = 60;
		//6D7464
		break;
	case TRACK_ELEM_DIAG_60_DEG_UP_TO_FLAT:
	case TRACK_ELEM_DIAG_FLAT_TO_60_DEG_DOWN:
		vertFactor = -60;
		//6d7470
		break;
	case TRACK_ELEM_LEFT_BARREL_ROLL_UP_TO_DOWN:
	case TRACK_ELEM_LEFT_BARREL_ROLL_DOWN_TO_UP:
		vertFactor = 170;
		lateralFactor = 115;
		//6d7581
		break;
	case TRACK_ELEM_RIGHT_BARREL_ROLL_UP_TO_DOWN:
	case TRACK_ELEM_RIGHT_BARREL_ROLL_DOWN_TO_UP:
		vertFactor = 170;
		lateralFactor = -115;
		//6d75A8
		break;
	case TRACK_ELEM_LEFT_BANK_TO_LEFT_QUARTER_TURN_3_TILES_25_DEG_UP:
		vertFactor = -(vehicle->track_progress / 2) + 134;
		lateralFactor = 90;
		//6d771C
		break;
	case TRACK_ELEM_RIGHT_BANK_TO_RIGHT_QUARTER_TURN_3_TILES_25_DEG_UP:
		vertFactor = -(vehicle->track_progress / 2) + 134;
		lateralFactor = -90;
		//6D7746
		break;
	case TRACK_ELEM_LEFT_QUARTER_TURN_3_TILES_25_DEG_DOWN_TO_LEFT_BANK:
		vertFactor = -(vehicle->track_progress / 2) + 134;
		lateralFactor = 90;
		//6D7731 identical to 6d771c
		break;
	case TRACK_ELEM_RIGHT_QUARTER_TURN_3_TILES_25_DEG_DOWN_TO_RIGHT_BANK:
		vertFactor = -(vehicle->track_progress / 2) + 134;
		lateralFactor = -90;
		//6D775B identical to 6d7746
		break;
	case TRACK_ELEM_LEFT_LARGE_HALF_LOOP_UP:
	case TRACK_ELEM_RIGHT_LARGE_HALF_LOOP_UP:
		vertFactor = (((uint16)(-(vehicle->track_progress - 311))) / 4) + 46;
		//6d7666
		break;
	case TRACK_ELEM_RIGHT_LARGE_HALF_LOOP_DOWN:
	case TRACK_ELEM_LEFT_LARGE_HALF_LOOP_DOWN:
		vertFactor = (vehicle->track_progress / 4) + 46;
		//6d767F
		break;
	case TRACK_ELEM_HEARTLINE_TRANSFER_UP:
		vertFactor = 103;
		if (vehicle->track_progress < 32)
			break;
		vertFactor = -103;
		if (vehicle->track_progress < 64)
			break;
		vertFactor = 0;
		if (vehicle->track_progress < 96)
			break;
		vertFactor = 103;
		if (vehicle->track_progress < 128)
			break;
		vertFactor = -103;
		//6d74A0
		break;
	case TRACK_ELEM_HEARTLINE_TRANSFER_DOWN:
		vertFactor = -103;
		if (vehicle->track_progress < 32)
			break;
		vertFactor = 103;
		if (vehicle->track_progress < 64)
			break;
		vertFactor = 0;
		if (vehicle->track_progress < 96)
			break;
		vertFactor = -103;
		if (vehicle->track_progress < 128)
			break;
		vertFactor = 103;
		//6D74CA
		break;
	case TRACK_ELEM_MULTIDIM_INVERTED_FLAT_TO_90_DEG_QUARTER_LOOP_DOWN:
	case TRACK_ELEM_INVERTED_FLAT_TO_90_DEG_QUARTER_LOOP_DOWN:
	case TRACK_ELEM_MULTIDIM_FLAT_TO_90_DEG_DOWN_QUARTER_LOOP:
		vertFactor = (vehicle->track_progress / 4) + 55;
		//6d762D
		break;
	case TRACK_ELEM_90_DEG_TO_INVERTED_FLAT_QUARTER_LOOP_UP:
	case TRACK_ELEM_MULTIDIM_90_DEG_UP_TO_INVERTED_FLAT_QUARTER_LOOP:
	case 255:
		vertFactor = (((uint16)(-(vehicle->track_progress - 137))) / 4) + 55;
		//6D7614
		break;
	case TRACK_ELEM_AIR_THRUST_TOP_CAP:
		vertFactor = -60;
		//6D744C
		break;
	case TRACK_ELEM_LEFT_BANKED_QUARTER_TURN_3_TILE_25_DEG_UP:
	case TRACK_ELEM_LEFT_BANKED_QUARTER_TURN_3_TILE_25_DEG_DOWN:
		vertFactor = 200;
		lateralFactor = 100;
		//6d76C8
		break;
	case TRACK_ELEM_RIGHT_BANKED_QUARTER_TURN_3_TILE_25_DEG_UP:
	case TRACK_ELEM_RIGHT_BANKED_QUARTER_TURN_3_TILE_25_DEG_DOWN:
		vertFactor = 200;
		lateralFactor = -100;
		//6d76d7
		break;
	case TRACK_ELEM_LEFT_BANKED_QUARTER_TURN_5_TILE_25_DEG_UP:
	case TRACK_ELEM_LEFT_BANKED_QUARTER_TURN_5_TILE_25_DEG_DOWN:
		vertFactor = 200;
		lateralFactor = 160;
		//6D76E6
		break;
	case TRACK_ELEM_RIGHT_BANKED_QUARTER_TURN_5_TILE_25_DEG_UP:
	case TRACK_ELEM_RIGHT_BANKED_QUARTER_TURN_5_TILE_25_DEG_DOWN:
		vertFactor = 200;
		lateralFactor = -160;
		//6d76F5
		break;
	}

	int gForceLateral = 0;

	if (vertFactor != 0) {
		gForceVert += abs(vehicle->velocity) * 98 / vertFactor;
	}

	if (lateralFactor != 0) {
		gForceLateral += abs(vehicle->velocity) * 98 / lateralFactor;
	}

	gForceVert *= 10;
	gForceLateral *= 10;
	gForceVert >>= 16;
	gForceLateral >>= 16;
	int eax, ebx, ecx, edx, esi, edi, ebp;

	esi = (int)vehicle;
	RCT2_CALLFUNC_X(0x006D73D0, &eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);

	if (gForceVert != (sint16)(eax & 0xFFFF))
		assert(gForceVert == (sint16)(eax & 0xFFFF));
	if (gForceLateral != (sint16)(edx & 0xFFFF))
		assert(gForceLateral == (sint16)(edx & 0xFFFF));
	if (verticalG != NULL) *verticalG = (sint16)(gForceVert & 0xFFFF);
	if (lateralG != NULL) *lateralG = (sint16)(gForceLateral & 0xFFFF);
}

void vehicle_set_map_toolbar(rct_vehicle *vehicle)
{
	rct_ride *ride;
	int vehicleIndex;

	ride = GET_RIDE(vehicle->ride);

	while (vehicle->is_child) {
		vehicle = GET_VEHICLE(vehicle->prev_vehicle_on_ride);
	}

	for (vehicleIndex = 0; vehicleIndex < 32; vehicleIndex++)
		if (ride->vehicles[vehicleIndex] == vehicle->sprite_index)
			break;

	RCT2_GLOBAL(RCT2_ADDRESS_MAP_TOOLTIP_ARGS + 0, uint16) = 2215;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_TOOLTIP_ARGS + 2, uint16) = 1165;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_TOOLTIP_ARGS + 4, uint16) = ride->name;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_TOOLTIP_ARGS + 6, uint32) = ride->name_arguments;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_TOOLTIP_ARGS + 10, uint16) = RideNameConvention[ride->type].vehicle_name + 2;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_TOOLTIP_ARGS + 12, uint16) = vehicleIndex + 1;

	int arg0, arg1;
	ride_get_status(vehicle->ride, &arg0, &arg1);
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_TOOLTIP_ARGS + 14, uint16) = (uint16)arg0;
	RCT2_GLOBAL(RCT2_ADDRESS_MAP_TOOLTIP_ARGS + 16, uint32) = (uint16)arg1;
}

rct_vehicle *vehicle_get_head(rct_vehicle *vehicle)
{
	rct_vehicle *prevVehicle;

	for (;;) {
		prevVehicle = GET_VEHICLE(vehicle->prev_vehicle_on_ride);
		if (prevVehicle->next_vehicle_on_train == SPRITE_INDEX_NULL)
			break;

		vehicle = prevVehicle;
	}

	return vehicle;
}

rct_vehicle *vehicle_get_tail(rct_vehicle *vehicle)
{
	uint16 spriteIndex;

	while ((spriteIndex = vehicle->next_vehicle_on_train) != SPRITE_INDEX_NULL) {
		vehicle = GET_VEHICLE(spriteIndex);
	}
	return vehicle;
}

int vehicle_is_used_in_pairs(rct_vehicle *vehicle)
{
	return vehicle->num_seats & VEHICLE_SEAT_PAIR_FLAG;
}

bool sub_6DF01A_loop(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);
	rct_xyz16 *unk_F64E20 = RCT2_ADDRESS(0x00F64E20, rct_xyz16);

	for (; vehicle->var_24 >= 13962; RCT2_GLOBAL(0x00F64E10, uint32)++) {
		uint8 trackType = vehicle->track_type >> 2;
		if (trackType == TRACK_ELEM_CABLE_LIFT_HILL &&
			vehicle->track_progress == 160) {
			RCT2_GLOBAL(0x00F64E18, uint32) |= (1 << 1);
		}

		uint16 trackProgress = vehicle->track_progress + 1;

		const rct_vehicle_info *moveInfo = vehicle_get_move_info(vehicle->var_CD, vehicle->track_type, 0);
		uint16 trackTotalProgress = *((uint16*)((int)moveInfo - 2));
		if (trackProgress >= trackTotalProgress) {
			RCT2_GLOBAL(0x00F64E36, uint8) = gTrackDefinitions[trackType].vangle_end;
			RCT2_GLOBAL(0x00F64E37, uint8) = gTrackDefinitions[trackType].bank_end;
			rct_map_element* trackElement =
				map_get_track_element_at_of_type_seq(
					vehicle->track_x,
					vehicle->track_y,
					vehicle->track_z / 8,
					trackType,
					0);

			rct_xy_element input;
			rct_xy_element output;
			int outputZ;
			int outputDirection;

			input.x = vehicle->track_x;
			input.y = vehicle->track_y;
			input.element = trackElement;

			if (!track_block_get_next(&input, &output, &outputZ, &outputDirection))
				return false;

			if (gTrackDefinitions[output.element->properties.track.type].vangle_start != RCT2_GLOBAL(0x00F64E36, uint8) ||
				gTrackDefinitions[output.element->properties.track.type].bank_start != RCT2_GLOBAL(0x00F64E37, uint8))
				return false;

			vehicle->track_x = output.x;
			vehicle->track_y = output.y;
			vehicle->track_z = outputZ;
			vehicle->track_direction = outputDirection;
			vehicle->track_type |= output.element->properties.track.type << 2;
			trackProgress = 0;
		}

		vehicle->track_progress = trackProgress;
		moveInfo = vehicle_get_move_info(vehicle->var_CD, vehicle->track_type, trackProgress);
		rct_xyz16 unk = {
			.x = moveInfo->x,
			.y = moveInfo->y,
			.z = moveInfo->z
		};

		unk.x += vehicle->track_x;
		unk.y += vehicle->track_y;
		unk.z += vehicle->track_z;

		uint8 bx = 0;
		unk.z += RCT2_GLOBAL(0x0097D21A + (ride->type * 8), sint8);
		if (unk.x != unk_F64E20->x)
			bx |= (1 << 0);
		if (unk.y != unk_F64E20->y)
			bx |= (1 << 1);
		if (unk.z != unk_F64E20->z)
			bx |= (1 << 2);

		vehicle->var_24 -= RCT2_ADDRESS(0x009A2930, sint32)[bx];
		unk_F64E20->x = unk.x;
		unk_F64E20->y = unk.y;
		unk_F64E20->z = unk.z;

		vehicle->sprite_direction = moveInfo->direction;
		vehicle->var_20 = moveInfo->var_08;
		vehicle->var_1F = moveInfo->var_07;

		if (vehicle->var_24 >= 13962) {
			vehicle->var_2C += RCT2_ADDRESS(0x009A2970, sint32)[vehicle->var_1F];
		}
	}
	return true;
}

bool sub_6DF21B_loop(rct_vehicle* vehicle) {
	rct_ride* ride = GET_RIDE(vehicle->ride);
	rct_xyz16 *unk_F64E20 = RCT2_ADDRESS(0x00F64E20, rct_xyz16);

	for (; vehicle->var_24 < 0; RCT2_GLOBAL(0x00F64E10, uint32)++) {
		uint16 trackProgress = vehicle->track_progress - 1;
		const rct_vehicle_info *moveInfo;

		if ((sint16)trackProgress == -1) {
			uint8 trackType = vehicle->track_type >> 2;
			RCT2_GLOBAL(0x00F64E36, uint8) = gTrackDefinitions[trackType].vangle_start;
			RCT2_GLOBAL(0x00F64E37, uint8) = gTrackDefinitions[trackType].bank_start;
			rct_map_element* trackElement =
				map_get_track_element_at_of_type_seq(
					vehicle->track_x,
					vehicle->track_y,
					vehicle->track_z / 8,
					trackType,
					0);

			rct_xy_element input;

			input.x = vehicle->track_x;
			input.y = vehicle->track_y;
			input.element = trackElement;
			track_begin_end output;

			if (!track_block_get_previous(input.x, input.y, input.element, &output))
				return false;

			if (gTrackDefinitions[output.begin_element->properties.track.type].vangle_end != RCT2_GLOBAL(0x00F64E36, uint8) ||
				gTrackDefinitions[output.begin_element->properties.track.type].bank_end != RCT2_GLOBAL(0x00F64E37, uint8))
				return false;

			vehicle->track_x = output.begin_x;
			vehicle->track_y = output.begin_y;
			vehicle->track_z = output.begin_z;
			vehicle->track_direction = output.begin_direction;
			vehicle->track_type |= output.begin_element->properties.track.type << 2;

			if (output.begin_element->properties.track.type == TRACK_ELEM_END_STATION) {
				RCT2_GLOBAL(0x00F64E18, uint32) = (1 << 0);
			}

			moveInfo = vehicle_get_move_info(vehicle->var_CD, vehicle->track_type, 0);
			uint16 trackTotalProgress = *((uint16*)((int)moveInfo - 2));
			trackProgress = trackTotalProgress - 1;
		}
		vehicle->track_progress = trackProgress;

		moveInfo = vehicle_get_move_info(vehicle->var_CD, vehicle->track_type, trackProgress);
		rct_xyz16 unk = {
			.x = moveInfo->x,
			.y = moveInfo->y,
			.z = moveInfo->z
		};

		unk.x += vehicle->track_x;
		unk.y += vehicle->track_y;
		unk.z += vehicle->track_z;

		uint8 bx = 0;
		unk.z += RCT2_GLOBAL(0x0097D21A + (ride->type * 8), sint8);
		if (unk.x != unk_F64E20->x)
			bx |= (1 << 0);
		if (unk.y != unk_F64E20->y)
			bx |= (1 << 1);
		if (unk.z != unk_F64E20->z)
			bx |= (1 << 2);

		vehicle->var_24 += RCT2_ADDRESS(0x009A2930, sint32)[bx];
		unk_F64E20->x = unk.x;
		unk_F64E20->y = unk.y;
		unk_F64E20->z = unk.z;

		vehicle->sprite_direction = moveInfo->direction;
		vehicle->var_20 = moveInfo->var_08;
		vehicle->var_1F = moveInfo->var_07;

		if (vehicle->var_24 < 0) {
			vehicle->var_2C += RCT2_ADDRESS(0x009A2970, sint32)[vehicle->var_1F];
		}
	}
	return true;
}

/**
 *
 *  rct2: 0x006DEF56
 */
int vehicle_update_track_motion_cable_lift(rct_vehicle *cableLift)
{
	rct_ride_type* rideEntry = GET_RIDE_ENTRY(cableLift->ride_subtype);
	rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[cableLift->vehicle_type];
	rct_ride* ride = GET_RIDE(cableLift->ride);

	RCT2_GLOBAL(0x00F64E2C, uint8) = 0;
	RCT2_GLOBAL(0x00F64E04, rct_vehicle*) = cableLift;
	RCT2_GLOBAL(0x00F64E18, uint32) = 0;
	RCT2_GLOBAL(0x00F64E1C, uint32) = (uint32)-1;

	cableLift->velocity += cableLift->var_2C;
	RCT2_GLOBAL(0x00F64E08, sint32) = cableLift->velocity;
	RCT2_GLOBAL(0x00F64E0C, sint32) = (cableLift->velocity / 1024) * 42;

	rct_vehicle* frontVehicle = cableLift;
	if (cableLift->velocity < 0) {
		frontVehicle = vehicle_get_tail(cableLift);
	}

	RCT2_GLOBAL(0x00F64E00, rct_vehicle*) = frontVehicle;

	for (rct_vehicle* vehicle = frontVehicle;;) {
		vehicle->var_2C = RCT2_ADDRESS(0x009A2970, sint32)[vehicle->var_1F];
		RCT2_GLOBAL(0x00F64E10, uint32) = 1;
		vehicle->var_24 += RCT2_GLOBAL(0x00F64E0C, sint32);

		if (vehicle->var_24 < 0 || vehicle->var_24 >= 13962) {
			rct_xyz16 *unk_F64E20 = RCT2_ADDRESS(0x00F64E20, rct_xyz16);
			unk_F64E20->x = vehicle->x;
			unk_F64E20->y = vehicle->y;
			unk_F64E20->z = vehicle->z;
			invalidate_sprite_2((rct_sprite*)vehicle);

			while (true) {
				if (vehicle->var_24 < 0) {
					if (sub_6DF21B_loop(vehicle) == true) {
						break;
					}
					else {
						RCT2_GLOBAL(0x00F64E18, uint32) |= (1 << 5);
						RCT2_GLOBAL(0x00F64E0C, uint32) -= vehicle->var_24 - 13962;
						vehicle->var_24 = 13962;
						vehicle->var_2C += RCT2_ADDRESS(0x009A2970, uint32)[vehicle->var_1F];
						RCT2_GLOBAL(0x00F64E10, uint32)++;
						continue;
					}
				}
				else {
					if (sub_6DF01A_loop(vehicle) == true) {
						break;
					}
					else {
						RCT2_GLOBAL(0x00F64E18, uint32) |= (1 << 5);
						RCT2_GLOBAL(0x00F64E0C, uint32) -= vehicle->var_24 + 1;
						vehicle->var_24 = -1;
						vehicle->var_2C += RCT2_ADDRESS(0x009A2970, uint32)[vehicle->var_1F];
						RCT2_GLOBAL(0x00F64E10, uint32)++;
					}
				}
			}
			sprite_move(
				unk_F64E20->x,
				unk_F64E20->y,
				unk_F64E20->z,
				(rct_sprite*)vehicle);

			invalidate_sprite_2((rct_sprite*)vehicle);
		}
		vehicle->var_2C /= RCT2_GLOBAL(0x00F64E10, uint32);
		if (RCT2_GLOBAL(0x00F64E08, sint32) >= 0) {
			if (vehicle->next_vehicle_on_train == 0xFFFF)
				break;
			vehicle = GET_VEHICLE(vehicle->next_vehicle_on_train);
		}
		else {
			if (vehicle == cableLift)
				break;
			vehicle = GET_VEHICLE(vehicle->prev_vehicle_on_ride);
		}
	}

	uint32 vehicleCount = 0;
	uint16 frictionTotal = 0;
	sint32 var2CTotal = 0;

	for (uint16 spriteId = cableLift->sprite_index; spriteId != 0xFFFF;) {
		rct_vehicle* vehicle = GET_VEHICLE(spriteId);
		vehicleCount++;
		frictionTotal += vehicle->friction;
		var2CTotal += vehicle->var_2C;
		spriteId = vehicle->next_vehicle_on_train;
	}

	sint32 ecx = (var2CTotal / vehicleCount) >> 9;
	ecx -= cableLift->velocity >> 12;

	sint32 edx = cableLift->velocity >> 8;
	edx *= edx;
	if (cableLift->velocity < 0)
		edx = -edx;
	edx >>= 4;
	ecx -= edx / frictionTotal;

	cableLift->var_2C = ecx;
	return RCT2_GLOBAL(0x00F64E18, uint32);
}

/* rct2: 0x006DA44E */
static int vehicle_update_motion_bumper_car(rct_vehicle* vehicle) {
	RCT2_GLOBAL(0x00F64E18, uint32) = 0;
	rct_ride* ride = GET_RIDE(vehicle->ride);

	sint32 nextVelocity = vehicle->velocity + vehicle->var_2C;
	if (ride->lifecycle_flags & (RIDE_LIFECYCLE_BREAKDOWN_PENDING | RIDE_LIFECYCLE_BROKEN_DOWN) &&
		ride->breakdown_reason_pending == BREAKDOWN_SAFETY_CUT_OUT) {
		nextVelocity = 0;
	}
	vehicle->velocity = nextVelocity;

	RCT2_GLOBAL(0x00F64E08, sint32) = nextVelocity;
	RCT2_GLOBAL(0x00F64E0C, sint32) = (nextVelocity / 1024) * 42;
	RCT2_GLOBAL(0x00F64E10, uint32) = 1;

	vehicle->var_2C = 0;
	if (!(ride->lifecycle_flags & (RIDE_LIFECYCLE_BREAKDOWN_PENDING | RIDE_LIFECYCLE_BROKEN_DOWN)) ||
		ride->breakdown_reason_pending != BREAKDOWN_SAFETY_CUT_OUT) {
		if (RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_TICKS, uint32) & 1 &&
			vehicle->var_34 != 0) {

			if (vehicle->var_34 > 0) {
				vehicle->var_34--;
				vehicle->sprite_direction += 2;
			}
			else {
				vehicle->var_34++;
				vehicle->sprite_direction -= 2;
			}
			vehicle->sprite_direction &= 0x1E;
			invalidate_sprite_2((rct_sprite*)vehicle);
		}
		else if ((scenario_rand() & 0xFFFF) <= 2849) {
			if (vehicle->var_35 & (1 << 6))
				vehicle->sprite_direction -= 2;
			else
				vehicle->sprite_direction += 2;
			vehicle->sprite_direction &= 0x1E;
			invalidate_sprite_2((rct_sprite*)vehicle);
		}
	}

	uint16 collideSprite = 0xFFFF;

	if (vehicle->var_C4 != 0) {
		uint8 oldC4 = vehicle->var_C4 & 0x1E;
		vehicle->var_C4 = 0;

		rct_xyz16 location = {
			.x = vehicle->x,
			.y = vehicle->y,
			.z = vehicle->z
		};
		
		location.x += RCT2_ADDRESS(0x009A36C4, sint16)[oldC4 * 4];
		location.y += RCT2_ADDRESS(0x009A36C6, sint16)[oldC4 * 4];
		location.x += RCT2_ADDRESS(0x009A36CC, sint16)[oldC4 * 4];
		location.y += RCT2_ADDRESS(0x009A36CE, sint16)[oldC4 * 4];

		if (!vehicle_update_bumper_car_collision(vehicle, location.x, location.y, &collideSprite)) {
			invalidate_sprite_2((rct_sprite*)vehicle);
			sprite_move(
				location.x,
				location.y,
				location.z,
				(rct_sprite*)vehicle
				);
			invalidate_sprite_2((rct_sprite*)vehicle);
		}
	}

	vehicle->var_24 += RCT2_GLOBAL(0x00F64E0C, sint32);

	if (vehicle->var_24 >= 13962) {
		vehicle->var_B8 &= ~(1 << 1);
		rct_xyz16 *unk_F64E20 = RCT2_ADDRESS(0x00F64E20, rct_xyz16);
		unk_F64E20->x = vehicle->x;
		unk_F64E20->y = vehicle->y;
		unk_F64E20->z = vehicle->z;

		invalidate_sprite_2((rct_sprite*)vehicle);

		while (1) {
			vehicle->var_35++;
			uint8 direction = vehicle->sprite_direction;
			direction |= vehicle->var_35 & 1;

			rct_xyz16 location = *unk_F64E20;
			location.x += RCT2_ADDRESS(0x009A36C4, sint16)[direction * 4];
			location.y += RCT2_ADDRESS(0x009A36C6, sint16)[direction * 4];

			if (vehicle_update_bumper_car_collision(vehicle, location.x, location.y, &collideSprite))
				break;

			vehicle->var_24 -= RCT2_ADDRESS(0x009A36C8, sint16)[direction * 4];
			unk_F64E20->x = location.x;
			unk_F64E20->y = location.y;
			if (vehicle->var_24 < 13962) {
				break;
			}
			RCT2_GLOBAL(0x00F64E10, uint32)++;
		}

		if (vehicle->var_24 >= 13962) {
			sint32 oldVelocity = vehicle->velocity;
			vehicle->var_24 = 0;
			vehicle->velocity = 0;
			uint8 direction = vehicle->sprite_direction | 1;

			if (collideSprite != 0xFFFF) {
				vehicle->var_34 = scenario_rand() & 1 ? 1 : -1;

				if (oldVelocity >= 131072) {
					rct_vehicle* collideVehicle = GET_VEHICLE(collideSprite);
					collideVehicle->var_C4 = direction;
					vehicle->var_C4 = direction ^ (1 << 4);
				}
			}
			else {
				vehicle->var_34 = scenario_rand() & 1 ? 6 : -6;

				if (oldVelocity >= 131072) {
					vehicle->var_C4 = direction ^ (1 << 4);
				}
			}
		}
		
		sprite_move(
			unk_F64E20->x,
			unk_F64E20->y,
			unk_F64E20->z,
			(rct_sprite*)vehicle
			);
		invalidate_sprite_2((rct_sprite*)vehicle);
	}
	
	sint32 eax = vehicle->velocity / 2;
	sint32 edx = vehicle->velocity >> 8;
	edx *= edx;
	if (vehicle->velocity < 0)
		edx = -edx;
	edx >>= 5;
	eax += edx;
	eax /= vehicle->friction;
	rct_ride_type* rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);
	rct_ride_type_vehicle* vehicleEntry = &rideEntry->vehicles[vehicle->vehicle_type];

	if (!(vehicleEntry->var_14 & (1 << 3))) {
		vehicle->var_2C = -eax;
		return RCT2_GLOBAL(0x00F64E18, uint32);
	}

	sint32 ebx = (vehicle->speed * vehicle->friction) >> 2;
	sint32 _eax = vehicle->speed << 14;
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_3) {
		_eax = -_eax;
	}
	_eax -= vehicle->velocity;
	_eax *= vehicle->acceleration * 2;
	_eax /= ebx;

	vehicle->var_2C = _eax - eax;
	return RCT2_GLOBAL(0x00F64E18, uint32);
}

rct_vehicle *cable_lift_segment_create(int rideIndex, int x, int y, int z, int direction, uint16 var_44, sint32 var_24, bool head)
{
	rct_ride *ride = GET_RIDE(rideIndex);
	rct_vehicle *current = &(create_sprite(1)->vehicle);
	current->sprite_identifier = SPRITE_IDENTIFIER_VEHICLE;
	current->ride = rideIndex;
	current->ride_subtype = 0xFF;
	if (head) {
		move_sprite_to_list((rct_sprite*)current, SPRITE_LINKEDLIST_OFFSET_VEHICLE);
		ride->cable_lift = current->sprite_index;
	}
	current->is_child = head ? 0 : 1;
	current->var_44 = var_44;
	current->var_24 = var_24;
	current->sprite_width = 10;
	current->sprite_height_negative = 10;
	current->sprite_height_positive = 10;
	current->friction = 100;
	current->num_seats = 0;
	current->speed = 20;
	current->acceleration = 80;
	current->velocity = 0;
	current->var_2C = 0;
	current->var_4A = 0;
	current->var_4C = 0;
	current->var_4E = 0;
	current->restraints_position = 0;
	current->var_BA = 0;
	current->var_B6 = 0;
	current->var_B8 = 0;
	current->sound1_id = 0xFF;
	current->sound2_id = 0xFF;
	current->var_C4 = 0;
	current->var_C5 = 0;
	current->var_C8 = 0;
	current->var_CA = 0;
	current->scream_sound_id = 0xFF;
	current->var_1F = 0;
	current->var_20 = 0;
	for (int j = 0; j < 32; j++) {
		current->peep[j] = SPRITE_INDEX_NULL;
	}
	current->var_CD = 0;
	current->sprite_direction = direction << 3;
	current->track_x = x;
	current->track_y = y;

	z = z * 8;
	current->track_z = z;
	z += RCT2_GLOBAL(0x0097D21A + (ride->type * 8), uint8);

	sprite_move(16, 16, z, (rct_sprite*)current);
	current->track_type = (TRACK_ELEM_CABLE_LIFT_HILL << 2) | (current->sprite_direction >> 3);
	current->track_progress = 164;
	current->update_flags = VEHICLE_UPDATE_FLAG_1;
	current->status = VEHICLE_STATUS_MOVING_TO_END_OF_STATION;
	current->sub_state = 0;
	current->num_peeps = 0;
	current->next_free_seat = 0;
	return current;
}

/**
 *
 *  rct2: 0x006DD365
 */
static bool vehicle_update_bumper_car_collision(rct_vehicle *vehicle, sint16 x, sint16 y, uint16 *spriteId)
{
	uint16 bp = (vehicle->var_44 * 30) >> 9;
	uint32 trackType = vehicle->track_type >> 2;
	
	sint16 rideLeft = vehicle->track_x + RCT2_ADDRESS(0x0099E228, uint8)[trackType * 4];
	sint16 rideRight = vehicle->track_x + RCT2_ADDRESS(0x0099E22A, uint8)[trackType * 4];
	sint16 rideTop = vehicle->track_y + RCT2_ADDRESS(0x0099E229, uint8)[trackType * 4];
	sint16 rideBottom = vehicle->track_y + RCT2_ADDRESS(0x0099E22B, uint8)[trackType * 4];

	if (x - bp < rideLeft ||
		y - bp < rideTop ||
		x + bp > rideRight ||
		y + bp > rideBottom) {
		if (spriteId != NULL)
			*spriteId = 0xFFFF;
		return true;
	}

	uint16 location = (y / 32) | ((x / 32) << 8);


	uint8 rideIndex = vehicle->ride;
	for (sint32* ebp = RCT2_ADDRESS(0x009A37C4, sint32); ebp <= RCT2_ADDRESS(0x009A37E4, sint32); ebp++) {
		uint16 spriteIdx = RCT2_ADDRESS(0xF1EF60, uint16)[location];
		for (rct_vehicle* vehicle2 = GET_VEHICLE(spriteIdx); spriteIdx != 0xFFFF; spriteIdx = vehicle2->next_in_quadrant) {
			vehicle2 = GET_VEHICLE(spriteIdx);

			if (vehicle2 == vehicle)
				continue;

			if (vehicle2->sprite_identifier != SPRITE_IDENTIFIER_VEHICLE)
				continue;

			if (vehicle2->ride != rideIndex)
				continue;

			int distX = abs(x - vehicle2->x);
			if (distX > 32768)
				continue;

			int distY = abs(y - vehicle2->y);
			if (distY > 32768)
				continue;

			int ecx = (vehicle->var_44 + vehicle2->var_44) / 2;
			ecx *= 30;
			ecx >>= 8;
			if (max(distX, distY) < ecx) {
				if (spriteId != NULL)
					*spriteId = vehicle2->sprite_index;
				return true;
			}
		}
		location += *ebp;
	}

	return false;
}

// 0x0x009A2970
const sint32 *dword_9A2970 = (sint32*)0x009A2970;

/**
 *
 *  rct2: 0x006DAB90
 */
static void vehicle_update_track_motion_up_stop_check(rct_vehicle *vehicle)
{
	rct_ride_type_vehicle *vehicleEntry = vehicle_get_vehicle_entry(vehicle);
	int verticalG, lateralG;

	// No up stops (coaster types)
	if (vehicleEntry->var_12 & (1 << 1)) {
		int trackType = vehicle->track_type >> 2;
		if (!track_element_is_covered(trackType)) {
			vehicle_get_g_forces(vehicle, &verticalG, &lateralG);
			lateralG = abs(lateralG);
			if (lateralG <= 150) {
				if (dword_9A2970[vehicle->var_1F] < 0) {
					if (verticalG > -40) {
						return;
					}
				} else if (verticalG > -80) {
					return;
				}
			}

			if (vehicle->var_1F != 8) {
				RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_DERAILED;
			}
		}
	} else if (vehicleEntry->var_12 & (1 << 2)) {
		// No up stops bobsleigh type
		int trackType = vehicle->track_type >> 2;
		if (!track_element_is_covered(trackType)) {
			vehicle_get_g_forces(vehicle, &verticalG, &lateralG);

			if (dword_9A2970[vehicle->var_1F] < 0) {
				if (verticalG > -45) {
					return;
				}
			} else {
				if (verticalG > -80) {
					return;
				}
			}

			if (vehicle->var_1F != 8 && vehicle->var_1F != 55) {
				RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_DERAILED;
			}
		}
	}
}

/**
 *
 *  rct2: 0x006DAC43
 */
static void sub_6DAB4C_chunk_2(rct_vehicle *vehicle)
{
	rct_ride *ride = GET_RIDE(vehicle->ride);
	rct_ride_type_vehicle *vehicleEntry = vehicle_get_vehicle_entry(vehicle);

	// Is chair lift type
	if (vehicleEntry->var_14 & (1 << 12)) {
		sint32 velocity = ride->speed << 16;
		if (RCT2_GLOBAL(0x00F64E34, uint8) == 0) {
			velocity = 0;
		}
		vehicle->velocity = velocity;
		vehicle->var_2C = 0;
	}
	
	int trackType = vehicle->track_type >> 2;
	switch (trackType) {
	case TRACK_ELEM_END_STATION:
	case TRACK_ELEM_BLOCK_BRAKES:
		if (ride->mode == RIDE_MODE_CONTINUOUS_CIRCUIT || ride_is_block_sectioned(ride)) {
			break;
		}
		return;
	case TRACK_ELEM_25_DEG_UP_TO_FLAT:
	case TRACK_ELEM_60_DEG_UP_TO_FLAT:
	case TRACK_ELEM_CABLE_LIFT_HILL:
	case TRACK_ELEM_DIAG_25_DEG_UP_TO_FLAT:
	case TRACK_ELEM_DIAG_60_DEG_UP_TO_FLAT:
		if (ride_is_block_sectioned(ride)) {
			break;
		}
		return;
	default:
		return;
	}

	rct_map_element *trackElement =  map_get_track_element_at_of_type(
		vehicle->track_x,
		vehicle->track_y,
		vehicle->track_z >> 3,
		trackType
	);
	if (trackType == TRACK_ELEM_END_STATION) {
		if (trackElement->flags & (1 << 5)) {
			RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_10;
		}
	} else if (trackType == TRACK_ELEM_CABLE_LIFT_HILL || trackType == TRACK_ELEM_BLOCK_BRAKES || track_element_is_lift_hill(trackElement)) {
		if (!(trackElement->flags & (1 << 5))) {
			if (trackType == TRACK_ELEM_BLOCK_BRAKES && vehicle->velocity >= 0) {
				if (vehicle->velocity <= 0x20364) {
					vehicle->velocity = 0x20364;
					vehicle->var_2C = 0;
				} else {
					vehicle->velocity -= vehicle->velocity >> 4;
					vehicle->var_2C = 0;
				}
			}
			return;
		}
		RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_10;
		vehicle->var_2C = 0;
		if (vehicle->velocity <= 0x20000) {
			vehicle->velocity = 0;
		}
		vehicle->velocity -= vehicle->velocity >> 3;
	}
}

/**
 *
 *  rct2: 0x006DADAE
 */
static void sub_6DAB4C_chunk_3(rct_vehicle *vehicle)
{
	sint32 nextVelocity = vehicle->var_2C + vehicle->velocity;
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_7) {
		nextVelocity = 0;
	}
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_10) {
		vehicle->var_D2--;
		if (vehicle->var_D2 == -70) {
			vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_10;
		}
		if (vehicle->var_D2 >= 0) {
			nextVelocity = 0;
			vehicle->var_2C = 0;
		}
	}
	vehicle->velocity = nextVelocity;

	RCT2_GLOBAL(0x00F64E08, sint32) = nextVelocity;
	RCT2_GLOBAL(0x00F64E0C, sint32) = (nextVelocity >> 10) * 42;
}

static void loc_6DB1B0(rct_vehicle *vehicle, rct_map_element *mapElement)
{
	int x = vehicle->track_x;
	int y = vehicle->track_y;
	int z = vehicle->track_z;
	track_begin_end trackBeginEnd;
	do {
		if (!track_block_get_previous(x, y, mapElement, &trackBeginEnd)) {
			return;
		}
		if (trackBeginEnd.begin_x == vehicle->track_x && trackBeginEnd.begin_y == vehicle->track_y) {
			return;
		}

		x = trackBeginEnd.begin_x;
		y = trackBeginEnd.begin_y;
		z = trackBeginEnd.begin_z;
	} while (track_element_is_block_start(trackBeginEnd.begin_element));

	mapElement = map_get_track_element_at(x, y, z >> 3);
	mapElement->flags &= ~(1 << 5);
	map_invalidate_element(x, y, mapElement);

	int trackType = mapElement->properties.track.type;
	if (trackType == 216 || trackType == TRACK_ELEM_END_STATION) {
		rct_ride *ride = GET_RIDE(vehicle->ride);
		if (ride_is_block_sectioned(ride)) {
			audio_play_sound_at_location(SOUND_48, x, y, z);
		}
	}
}

/**
 *
 *  rct2: 0x006D6776
 */
static void sub_6D6776(rct_vehicle *vehicle)
{
	rct_ride_type_vehicle *vehicleEntry = vehicle_get_vehicle_entry(vehicle);
	RCT2_CALLPROC_X(0x006D6776, 0, 0, 0, 0, (int)vehicle, (int)vehicleEntry, 0);
}

#pragma region off_9A2E84

enum {
	loc_6D673C,
	loc_6D66F0,
	loc_6D672B,
	loc_6D6711,
	loc_6D66D6,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D669A,
	loc_6D6684,
	loc_6D665A,
	loc_6D6708,
	loc_6D6703,
	loc_6D66DD,
	loc_6D6718
};

static const uint8 off_9A2E84[256] = {
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D66F0,
	loc_6D672B,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D66F0,
	loc_6D672B,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D66F0,
	loc_6D672B,
	loc_6D66F0,
	loc_6D672B,
	loc_6D6711,
	loc_6D66D6,
	loc_6D673C,
	loc_6D673C,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D669A,
	loc_6D6684,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D66F0,
	loc_6D672B,
	loc_6D6711,
	loc_6D66D6,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D66F0,
	loc_6D672B,
	loc_6D66F0,
	loc_6D672B,
	loc_6D669A,
	loc_6D6684,
	loc_6D669A,
	loc_6D6684,
	loc_6D673C,
	loc_6D665A,
	loc_6D673C,
	loc_6D66F0,
	loc_6D672B,
	loc_6D66F0,
	loc_6D672B,
	loc_6D66F0,
	loc_6D672B,
	loc_6D66F0,
	loc_6D672B,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D6708,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D6703,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D66DD,
	loc_6D6718,
	loc_6D66DD,
	loc_6D6718,
	loc_6D66DD,
	loc_6D6718,
	loc_6D66DD,
	loc_6D6718,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D66C3,
	loc_6D66B0,
	loc_6D66F0,
	loc_6D672B,
	loc_6D66F0,
	loc_6D672B,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C,
	loc_6D673C
};

#pragma endregion

/**
 *
 *  rct2: 0x006D661F
 */
static void sub_6D661F(rct_vehicle *vehicle)
{
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_13) {
		vehicle->var_B6 = 0;
		return;
	}

	rct_ride_type_vehicle *vehicleEntry = vehicle_get_vehicle_entry(vehicle);
	int spinningInertia = vehicleEntry->spinning_inertia;
	int trackType = vehicle->track_type >> 2;
	sint32 dword_F64E08 = RCT2_GLOBAL(0x00F64E08, sint32);
	int unk;
	switch (off_9A2E84[trackType]) {
	case loc_6D665A:
		spinningInertia += 6;
		unk = dword_F64E08 >> spinningInertia;
		if (vehicle->sprite_index & 1) {
			vehicle->var_B6 -= unk;
		} else {
			vehicle->var_B6 += unk;
		}
		break;
	case loc_6D6684:
	case loc_6D6703:
		spinningInertia += 5;
		vehicle->var_B6 -= dword_F64E08 >> spinningInertia;
		break;
	case loc_6D669A:
		spinningInertia += 5;
		vehicle->var_B6 += dword_F64E08 >> spinningInertia;
		break;
	case loc_6D66B0:
		spinningInertia += 7;
		vehicle->var_B6 -= dword_F64E08 >> spinningInertia;
		break;
	case loc_6D66C3:
		spinningInertia += 7;
		vehicle->var_B6 += dword_F64E08 >> spinningInertia;
		break;
	case loc_6D66D6:
		if (vehicle->track_progress < 48) {
			spinningInertia += 8;
			vehicle->var_B6 += dword_F64E08 >> spinningInertia;
			break;
		}
	case loc_6D66DD:
		spinningInertia += 9;
		vehicle->var_B6 += dword_F64E08 >> spinningInertia;
		break;
	case loc_6D66F0:
		spinningInertia += 8;
		vehicle->var_B6 += dword_F64E08 >> spinningInertia;
		break;
	case loc_6D6708:
		if (vehicle->track_progress > 22) {
			spinningInertia += 5;
			vehicle->var_B6 += dword_F64E08 >> spinningInertia;
		}
		break;
	case loc_6D6711:
		if (vehicle->track_progress < 48) {
			spinningInertia += 8;
			vehicle->var_B6 += dword_F64E08 >> spinningInertia;
			break;
		}
	case loc_6D6718:
		spinningInertia += 9;
		vehicle->var_B6 -= dword_F64E08 >> spinningInertia;
		break;
	case loc_6D672B:
		spinningInertia += 8;
		vehicle->var_B6 += dword_F64E08 >> spinningInertia;
		break;
	}

	unk = clamp(-0x600, vehicle->var_B6, 0x600);
	vehicle->var_B6 = unk;
	vehicle->var_BA += unk >> 8;
	vehicle->var_B6 -= unk >> vehicleEntry->spinning_friction;
	invalidate_sprite_2((rct_sprite*)vehicle);
}

/**
 *
 *  rct2: 0x006D63D4
 */
static void sub_6D63D4(rct_vehicle *vehicle)
{
	RCT2_CALLPROC_X(0x006D63D4, 0, 0, 0, 0, (int)vehicle, 0, 0);
}

/**
 *
 *  rct2: 0x006DEDB1
 */
static vehicle_play_scenery_door_open_sound(rct_vehicle *vehicle, rct_map_element *mapElement)
{
	rct_scenery_entry *wallEntry = g_wallSceneryEntries[mapElement->properties.fence.type];
	int doorSoundType = (wallEntry->wall.flags2 >> 1) & 3;
	if (doorSoundType != 0) {
		int soundId = DoorOpenSoundIds[doorSoundType];
		if (soundId != 255) {
			audio_play_sound_at_location(doorSoundType, vehicle->x, vehicle->track_y, vehicle->track_z);
		}
	}
}

/**
 *
 *  rct2: 0x006DED7A
 */
static vehicle_play_scenery_door_close_sound(rct_vehicle *vehicle, rct_map_element *mapElement)
{
	rct_scenery_entry *wallEntry = g_wallSceneryEntries[mapElement->properties.fence.type];
	int doorSoundType = (wallEntry->wall.flags2 >> 1) & 3;
	if (doorSoundType != 0) {
		int soundId = DoorCloseSoundIds[doorSoundType];
		if (soundId != 255) {
			audio_play_sound_at_location(doorSoundType, vehicle->x, vehicle->track_y, vehicle->track_z);
		}
	}
}

/**
 *
 *  rct2: 0x006DEE93
 */
static void vehicle_update_scenery_door(rct_vehicle *vehicle)
{
	int trackType = vehicle->track_type >> 2;
	rct_preview_track *trackBlock = TrackBlocks[trackType];
	while ((trackBlock + 1)->index != 255) {
		trackBlock++;
	}
	rct_track_coordinates *trackCoordinates = &TrackCoordinates[trackType];
	int x = floor2(vehicle->x, 32);
	int y = floor2(vehicle->y, 32);
	int z = (vehicle->track_z - trackBlock->z + trackCoordinates->z_end) >> 3;
	int direction = (vehicle->track_direction + trackCoordinates->rotation_end) & 3;

	rct_map_element *mapElement = map_get_fence_element_at(x, y, z, direction);
	if (mapElement == NULL) {
		return;
	}

	if (vehicle->next_vehicle_on_train != SPRITE_INDEX_NULL) {
		mapElement->properties.fence.item[3] &= 7;
		mapElement->properties.fence.item[3] |= 8;
		map_animation_create(MAP_ANIMATION_TYPE_WALL_UNKNOWN, x, y, z);
		vehicle_play_scenery_door_open_sound(vehicle, mapElement);
	} else {
		mapElement->properties.fence.item[3] &= 7;
		mapElement->properties.fence.item[3] |= 0x30;
		vehicle_play_scenery_door_close_sound(vehicle, mapElement);
	}
}

/**
 *
 *  rct2: 0x006DB38B
 */
static bool loc_6DB38B(rct_vehicle *vehicle, rct_map_element *mapElement)
{
	// Get bank
	int bankStart = track_get_actual_bank_3(vehicle, mapElement);
	
	// Get vangle
	int trackType = mapElement->properties.track.type;
	int vangleStart = gTrackDefinitions[trackType].vangle_start;

	// ?
	uint16 angleAndBank = vangleStart | (bankStart << 8);
	if (angleAndBank != RCT2_GLOBAL(0x00F64E36, uint16)) {
		return false;
	}

	return true;
}

void loc_6DB481(rct_vehicle *vehicle)
{
	uint16 probability = 0x8000;
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_6) {
		vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_6;
	} else {
		probability = 0x0A3D;
	}
	if ((scenario_rand() & 0xFFFF) <= probability) {
		vehicle->var_CD += 2;
	}
}

/**
 *
 *  rct2: 0x006DB545
 */
static void vehicle_trigger_on_ride_photo(rct_vehicle *vehicle, rct_map_element *mapElement)
{
	mapElement->properties.track.sequence &= 0x0F;
	mapElement->properties.track.sequence |= 0x30;
	map_animation_create(
		MAP_ANIMATION_TYPE_TRACK_ONRIDEPHOTO,
		vehicle->track_x,
		vehicle->track_y,
		mapElement->base_height
	);
}

/**
 *
 *  rct2: 0x006DEDE8
 */
static void sub_6DEDE8(rct_vehicle *vehicle)
{
	int trackType = vehicle->track_type >> 2;
	rct_preview_track *trackBlock = TrackBlocks[trackType];
	rct_track_coordinates *trackCoordinates = &TrackCoordinates[trackType];
	int x = vehicle->track_x;
	int y = vehicle->track_y;
	int z = (vehicle->track_z - trackBlock->z + trackCoordinates->z_begin) >> 3;
	int direction = (vehicle->track_direction + trackCoordinates->rotation_begin) & 3;
	direction ^= 2;

	rct_map_element *mapElement = map_get_fence_element_at(x, y, z, direction);
	if (mapElement == NULL) {
		return;
	}

	if (vehicle->next_vehicle_on_train != SPRITE_INDEX_NULL) {
		mapElement->properties.fence.item[3] &= 7;
		mapElement->properties.fence.item[3] |= 0x88;
		map_animation_create(MAP_ANIMATION_TYPE_WALL_UNKNOWN, x, y, z);
		vehicle_play_scenery_door_open_sound(vehicle, mapElement);
	} else {
		mapElement->properties.fence.item[3] &= 7;
		mapElement->properties.fence.item[3] |= 0xB0;
		vehicle_play_scenery_door_close_sound(vehicle, mapElement);
	}
}

static void vehicle_update_play_water_splash_sound()
{
	if (RCT2_GLOBAL(0x00F64E08, sint32) <= 0x20364) {
		return;
	}

	audio_play_sound_at_location(
		SOUND_WATER_SPLASH,
		unk_F64E20->x,
		unk_F64E20->y,
		unk_F64E20->z
	);
}

/**
 *
 *  rct2: 0x006DB59E
 */
static void vehicle_update_handle_water_splash(rct_vehicle *vehicle)
{
	rct_ride_type *rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);
	int trackType = vehicle->track_type >> 2;

	if (!(rideEntry->flags & RIDE_ENTRY_FLAG_8)) {
		if (rideEntry->flags & RIDE_ENTRY_FLAG_9) {
			if (!vehicle->is_child) {
				if (track_element_is_covered(trackType)) {
					rct_vehicle *nextVehicle = GET_VEHICLE(vehicle->next_vehicle_on_ride);
					rct_vehicle *nextNextVehicle = GET_VEHICLE(nextVehicle->next_vehicle_on_ride);
					if (!track_element_is_covered(nextNextVehicle->track_type >> 2)) {
						if (vehicle->track_progress == 4) {
							vehicle_update_play_water_splash_sound();
						}
					}
				}
			}
		}
	} else {
		if (trackType == TRACK_ELEM_25_DEG_DOWN_TO_FLAT) {
			if (vehicle->track_progress == 12) {
				vehicle_update_play_water_splash_sound();
			}
		}
	}
	if (!vehicle->is_child) {
		if (trackType == TRACK_ELEM_WATER_SPLASH) {
			if (vehicle->track_progress == 48) {
				vehicle_update_play_water_splash_sound();
			}
		}
	}
}

/**
 *
 *  rct2: 0x006DB807
 */
static void sub_6DB807(rct_vehicle *vehicle)
{
	const rct_vehicle_info *moveInfo = vehicle_get_move_info(
		vehicle->var_CD,
		vehicle->track_type,
		vehicle->track_progress
	);
	int x = vehicle->track_x + moveInfo->x;
	int y = vehicle->track_y + moveInfo->y;
	int z = vehicle->z;
	sprite_move(x, y, z, (rct_sprite*)vehicle);
}

/** Collision?
 *
 *  rct2: 0x006DD078
 * @param vehicle (esi)
 * @param otherVehicleIndex (bp)
 */
static bool sub_6DD078(rct_vehicle *vehicle, uint16 otherVehicleIndex)
{
	return RCT2_CALLPROC_X(0x006DD078, 0, 0, 0, 0, (int)vehicle, 0, otherVehicleIndex) & 0x100;
}

/**
 *
 *  rct2: 0x006DB7D6
 */
static void sub_6DB7D6(rct_vehicle *vehicle)
{
	rct_vehicle *previousVehicle = GET_VEHICLE(vehicle->prev_vehicle_on_ride);
	rct_vehicle *nextVehicle = GET_VEHICLE(vehicle->next_vehicle_on_ride);

	vehicle->track_progress = 168;
	vehicle->vehicle_type ^= 1;

	previousVehicle->track_progress = 86;
	nextVehicle->track_progress = 158;

	sub_6DB807(nextVehicle);
	sub_6DB807(previousVehicle);
}

/**
 *
 *  rct2: 0x006DBF3E
 */
void sub_6DBF3E(rct_vehicle *vehicle)
{
	rct_ride_type_vehicle *vehicleEntry = vehicle_get_vehicle_entry(vehicle);

	vehicle->var_2C = (uint32)((sint32)vehicle->var_2C / RCT2_GLOBAL(0x00F64E10, sint32));
	if (vehicle->var_CD == 2) {
		return;
	}

	int trackType = vehicle->track_type >> 2;
	if (!(RCT2_GLOBAL(0x0099BA64 + (trackType * 16), uint32) & 0x10)) {
		return;
	}

	RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_3;

	rct_map_element *mapElement = map_get_track_element_at_of_type_seq(
		vehicle->track_x,
		vehicle->track_y,
		vehicle->track_z >> 3,
		trackType,
		0
	);
	if (RCT2_GLOBAL(0x00F64E1C, uint32) == 0xFFFFFFFF) {
		RCT2_GLOBAL(0x00F64E1C, uint32) = (mapElement->properties.track.sequence >> 4) & 7;
	}

	if (trackType == TRACK_ELEM_TOWER_BASE &&
		vehicle == RCT2_GLOBAL(0x00F64E04, rct_vehicle*)
	) {
		if (vehicle->track_progress > 3 && !(vehicle->update_flags & VEHICLE_UPDATE_FLAG_3)) {
			rct_xy_element input, output;
			int outputZ, outputDirection;

			input.x = vehicle->track_x;
			input.y = vehicle->track_y;
			input.element = mapElement;
			if (track_block_get_next(&input, &output, &outputZ, &outputDirection)) {
				RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_12;
			}
		}

		if (vehicle->track_progress <= 3) {
			RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_AT_STATION;
		}
	}

	if (trackType != TRACK_ELEM_END_STATION ||
		vehicle != RCT2_GLOBAL(0x00F64E04, rct_vehicle*)
	) {
		return;
	}

	uint16 ax = vehicle->track_progress;
	if (RCT2_GLOBAL(0x00F64E08, sint32) < 0) {
		if (ax <= 22) {
			RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_AT_STATION;
		}
	} else {
		uint16 cx = 17;
		if (vehicleEntry->var_14 & (1 << 12)) {
			cx = 6;
		}
		if (vehicleEntry->var_14 & (1 << 14)) {
			cx = vehicle->var_CD == 6 ? 18 : 20;
		}

		if (ax > cx) {
			RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_AT_STATION;
		}
	}
}

/**
 *
 *  rct2: 0x006DB08C
 */
bool vehicle_update_track_motion_forwards_get_new_track(rct_vehicle *vehicle, uint16 trackType, rct_ride* ride, rct_ride_type* rideEntry) {
	registers regs = { 0 };

	RCT2_GLOBAL(0x00F64E36, uint8) = gTrackDefinitions[trackType].vangle_end;
	RCT2_GLOBAL(0x00F64E37, uint8) = gTrackDefinitions[trackType].bank_end;
	rct_map_element *mapElement = map_get_track_element_at_of_type_seq(
		vehicle->track_x,
		vehicle->track_y,
		vehicle->track_z >> 3,
		trackType,
		0
		);
	if (trackType == TRACK_ELEM_CABLE_LIFT_HILL && vehicle == RCT2_GLOBAL(0x00F64E04, rct_vehicle*)) {
		RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_11;
	}

	if (track_element_is_block_start(mapElement)) {
		if (vehicle->next_vehicle_on_train == SPRITE_INDEX_NULL) {
			mapElement->flags |= (1 << 5);
			if (trackType == 216 || trackType == TRACK_ELEM_END_STATION) {
				if (!(rideEntry->vehicles[0].var_14 & (1 << 3))) {
					audio_play_sound_at_location(SOUND_49, vehicle->track_x, vehicle->track_y, vehicle->track_z);
				}
			}
			map_invalidate_element(vehicle->track_x, vehicle->track_z, mapElement);
			loc_6DB1B0(vehicle, mapElement);
		}
	}

	// TODO check if getting the vehicle entry again is necessary
	rct_ride_type_vehicle* vehicleEntry = vehicle_get_vehicle_entry(vehicle);

	if (vehicleEntry->var_12 & (1 << 8)) {
		vehicle_update_scenery_door(vehicle);
	}

	switch (vehicle->var_CD) {
	default:
		goto loc_6DB358;
	case 2:
	case 3:
		vehicle->var_CD = 2;
		goto loc_6DB32A;
	case 4:
		vehicle->var_CD = 1;
		goto loc_6DB358;
	case 7:
		vehicle->var_CD = 6;
		goto loc_6DB358;
	case 8:
		vehicle->var_CD = 5;
		goto loc_6DB358;
	}

loc_6DB32A:
	{
		track_begin_end trackBeginEnd;
		if (!track_block_get_previous(vehicle->track_x, vehicle->track_y, mapElement, &trackBeginEnd)) {
			return false;
		}
		regs.eax = trackBeginEnd.begin_x;
		regs.ecx = trackBeginEnd.begin_y;
		regs.edx = trackBeginEnd.begin_z;
		regs.bl = trackBeginEnd.begin_direction;
	}
	goto loc_6DB41D;

loc_6DB358:
	{
		rct_xy_element xyElement;
		int z, direction;
		xyElement.x = vehicle->track_x;
		xyElement.y = vehicle->track_y;
		xyElement.element = mapElement;
		if (!track_block_get_next(&xyElement, &xyElement, &z, &direction)) {
			return false;
		}
		mapElement = xyElement.element;
		regs.eax = xyElement.x;
		regs.ecx = xyElement.y;
		regs.edx = z;
		regs.bl = direction;
	}
	if (mapElement->properties.track.type == 211 ||
		mapElement->properties.track.type == 212
		) {
		if (!vehicle->is_child && vehicle->velocity <= 0x30000) {
			vehicle->velocity = 0;
		}
	}

	if (!loc_6DB38B(vehicle, mapElement)) {
		return false;
	}

	// Update VEHICLE_UPDATE_FLAG_11 flag
	vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_11;
	int rideType = GET_RIDE(mapElement->properties.track.ride_index)->type;
	if (RideData4[rideType].flags & RIDE_TYPE_FLAG4_3) {
		if (mapElement->properties.track.colour & 4) {
			vehicle->update_flags |= VEHICLE_UPDATE_FLAG_11;
		}
	}

loc_6DB41D:
	vehicle->track_x = regs.ax;
	vehicle->track_y = regs.cx;
	vehicle->track_z = regs.dx;

	// TODO check if getting the vehicle entry again is necessary
	vehicleEntry = vehicle_get_vehicle_entry(vehicle);

	if ((vehicleEntry->var_14 & (1 << 14)) && vehicle->var_CD < 7) {
		trackType = mapElement->properties.track.type;
		if (trackType == TRACK_ELEM_FLAT) {
			loc_6DB481(vehicle);
		}
		else if (ride->lifecycle_flags & RIDE_LIFECYCLE_PASS_STATION_NO_STOPPING) {
			if (track_element_is_station(mapElement)) {
				loc_6DB481(vehicle);
			}
		}
	}

	if (vehicle->var_CD != 0 && vehicle->var_CD < 5) {
		regs.ax >>= 5;
		regs.cx >>= 5;
		regs.ah = regs.cl;
		regs.dx >>= 3;
		if (regs.ax != ride->var_13C || regs.dl != ride->var_13F) {
			if (regs.ax == ride->var_13A && regs.dl == ride->var_13E) {
				vehicle->var_CD = 4;
			}
		}
		else {
			vehicle->var_CD = 3;
		}
	}

loc_6DB500:
	// Update VEHICLE_UPDATE_FLAG_0
	vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_0;
	if (track_element_is_lift_hill(mapElement)) {
		vehicle->update_flags |= VEHICLE_UPDATE_FLAG_0;
	}

	trackType = mapElement->properties.track.type;
	if (trackType != TRACK_ELEM_BRAKES) {
		vehicle->var_D9 = mapElement->properties.track.colour >> 4;
	}
	vehicle->track_direction = regs.bl & 3;
	vehicle->track_type |= trackType << 2;
	vehicle->var_CF = (mapElement->properties.track.sequence >> 3) & 0x1E;
	if (trackType == TRACK_ELEM_ON_RIDE_PHOTO) {
		vehicle_trigger_on_ride_photo(vehicle, mapElement);
	}
	if (trackType == TRACK_ELEM_ROTATION_CONTROL_TOGGLE) {
		vehicle->update_flags ^= VEHICLE_UPDATE_FLAG_13;
	}
	if (vehicleEntry->var_12 & (1 << 8)) {
		sub_6DEDE8(vehicle);
	}
	return true;
}

/**
 *
 *  rct2: 0x006DAB4C
 */
int vehicle_update_track_motion(rct_vehicle *vehicle, int *outStation)
{
	registers regs = { 0 };

	//////////////////////////////////////////////////////////////////////////////////////////
	regs.esi = (int)vehicle;
	RCT2_CALLFUNC_Y(0x006DAB4C, &regs);
	if (outStation != NULL) *outStation = regs.ebx;
	return regs.eax;
	//////////////////////////////////////////////////////////////////////////////////////////

	rct_ride *ride = GET_RIDE(vehicle->ride);
	rct_ride_type *rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);
	rct_ride_type_vehicle *vehicleEntry = vehicle_get_vehicle_entry(vehicle);
	
	rct_map_element *mapElement = NULL;

	// esi = vehicle
	// eax = rideEntry
	// edi = vehicleEntry

	if (vehicleEntry->var_12 & (1 << 3)) {
		goto loc_6DC3A7;
	}

	RCT2_GLOBAL(0x00F64E2C, uint8) = 0;
	RCT2_GLOBAL(0x00F64E04, rct_vehicle*) = vehicle;
	RCT2_GLOBAL(0x00F64E18, uint32) = 0;
	RCT2_GLOBAL(0x00F64E1C, uint32) = 0xFFFFFFFF;

	vehicle_update_track_motion_up_stop_check(vehicle);
	sub_6DAB4C_chunk_2(vehicle);
	sub_6DAB4C_chunk_3(vehicle);

	if (RCT2_GLOBAL(0x00F64E08, sint32) < 0) {
		vehicle = vehicle_get_tail(vehicle);
	}
	// This will be the front vehicle even when traveling
	// backwards.
	RCT2_GLOBAL(0x00F64E00, rct_vehicle*) = vehicle;

loc_6DAE27:
	// Swinging cars
	if (vehicleEntry->var_14 & (1 << 1)) {
		sub_6D6776(vehicle);
	}
	// Spinning cars
	if (vehicleEntry->var_14 & (1 << 2)) {
		sub_6D661F(vehicle);
	}
	// Rider sprites?? animation??
	if ((vehicleEntry->var_14 & (1 << 7)) || (vehicleEntry->var_14 & (1 << 8))) {
		sub_6D63D4(vehicle);
	}
	vehicle->var_2C = dword_9A2970[vehicle->var_1F];
	RCT2_GLOBAL(0x00F64E10, uint32) = 1;

	regs.eax = RCT2_GLOBAL(0x00F64E0C, sint32) + vehicle->var_24;
	vehicle->var_24 = regs.eax;
	if (regs.eax < 0) {
		// Backward loop
		goto loc_6DBA13;
	}
	if (regs.eax < 0x368A) {
		// Location found
		goto loc_6DBF3E;
	}
	// Forward loop
	vehicle->var_B8 &= ~(1 << 1);
	unk_F64E20->x = vehicle->x;
	unk_F64E20->y = vehicle->y;
	unk_F64E20->z = vehicle->z;
	invalidate_sprite_2((rct_sprite*)vehicle);

loc_6DAEB9:
	regs.edi = vehicle->track_type;
	regs.cx = vehicle->track_type >> 2;

	int trackType = vehicle->track_type >> 2;
	if (trackType == TRACK_ELEM_HEARTLINE_TRANSFER_UP || trackType == TRACK_ELEM_HEARTLINE_TRANSFER_DOWN) {
		if (vehicle->track_progress == 80) {
			vehicle->vehicle_type ^= 1;
			vehicleEntry = vehicle_get_vehicle_entry(vehicle);
		}
		if (RCT2_GLOBAL(0x00F64E08, sint32) >= 0x40000) {
			vehicle->var_2C = -RCT2_GLOBAL(0x00F64E08, sint32) * 8;
		} else if (RCT2_GLOBAL(0x00F64E08, sint32) < 0x20000) {
			vehicle->var_2C = 0x50000;
		}
	} else if (trackType == TRACK_ELEM_BRAKES) {
		if (!(
			ride->lifecycle_flags & RIDE_LIFECYCLE_BROKEN_DOWN &&
			ride->breakdown_reason_pending == BREAKDOWN_BRAKES_FAILURE &&
			ride->mechanic_status == RIDE_MECHANIC_STATUS_4
			)) {
			regs.eax = vehicle->var_CF << 16;
			if (regs.eax < RCT2_GLOBAL(0x00F64E08, sint32)) {
				vehicle->var_2C = -RCT2_GLOBAL(0x00F64E08, sint32) * 16;
			} else if (!(RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_TICKS, uint32) & 0x0F)) {
				if (RCT2_GLOBAL(0x00F64E2C, uint8) == 0) {
					RCT2_GLOBAL(0x00F64E2C, uint8)++;
					audio_play_sound_at_location(SOUND_51, vehicle->x, vehicle->y, vehicle->z);
				}
			}
		}
	}

	if ((trackType == TRACK_ELEM_FLAT && ride->type == RIDE_TYPE_REVERSE_FREEFALL_COASTER) ||
		(trackType == TRACK_ELEM_POWERED_LIFT)
		) {
		vehicle->var_2C = RCT2_GLOBAL(0x0097CF40 + (ride->type * 8) + 7, uint8) << 16;
	}
	if (trackType == TRACK_ELEM_BRAKE_FOR_DROP) {
		if (!vehicle->is_child) {
			if (!(vehicle->update_flags & VEHICLE_UPDATE_FLAG_10)) {
				if (vehicle->track_progress >= 8) {
					vehicle->var_2C = -RCT2_GLOBAL(0x00F64E08, sint32) * 16;
					if (vehicle->track_progress >= 24) {
						vehicle->update_flags |= VEHICLE_UPDATE_FLAG_10;
						vehicle->var_D2 = 90;
					}
				}
			}
		}
	}
	if (trackType == TRACK_ELEM_LOG_FLUME_REVERSER) {
		if (vehicle->track_progress != 16 || vehicle->velocity < 0x40000) {
			if (vehicle->track_progress == 32) {
				vehicle->vehicle_type = vehicleEntry->var_58;
				vehicleEntry = vehicle_get_vehicle_entry(vehicle);
			}
		} else {
			vehicle->track_progress += 17;
		}
	}

	regs.ax = vehicle->track_progress + 1;
	
	const rct_vehicle_info *moveInfo = vehicle_get_move_info(
		vehicle->var_CD,
		vehicle->track_type,
		0
		);

	// Track Total Progress is in the two bytes before the move info list
	uint16 trackTotalProgress = *((uint16*)((int)moveInfo - 2));
	if (regs.ax >= trackTotalProgress) {
		if (!vehicle_update_track_motion_forwards_get_new_track(vehicle, trackType, ride, rideEntry)) {
			goto loc_6DB94A;
		}
		regs.ax = 0;
	}

	vehicle->track_progress = regs.ax;
	vehicle_update_handle_water_splash(vehicle);

loc_6DB706:;
	moveInfo = vehicle_get_move_info(
		vehicle->var_CD,
		vehicle->track_type,
		vehicle->track_progress
		);
	sint16 x = vehicle->track_x + moveInfo->x;
	sint16 y = vehicle->track_y + moveInfo->y;
	sint16 z = vehicle->track_z + moveInfo->z + RCT2_GLOBAL(0x0097D21A + (ride->type * 8), uint8);
	
	trackType = vehicle->track_type >> 2;
	regs.ebx = 0;
	if (x != unk_F64E20->x) { regs.ebx |= 1; }
	if (y != unk_F64E20->y) { regs.ebx |= 2; }
	if (z != unk_F64E20->z) { regs.ebx |= 4; }
	if (vehicle->var_CD == 15 &&
		(trackType >= TRACK_ELEM_LEFT_REVERSER ||
		trackType == TRACK_ELEM_RIGHT_REVERSER) &&
		vehicle->track_progress >= 30 &&
		vehicle->track_progress <= 66
		) {
		regs.ebx |= 8;
	}

	if (vehicle->var_CD == 16 &&
		(trackType >= TRACK_ELEM_LEFT_REVERSER ||
		trackType == TRACK_ELEM_RIGHT_REVERSER) &&
		vehicle->track_progress == 96
		) {
		sub_6DB7D6(vehicle);

		const rct_vehicle_info *moveInfo2 = vehicle_get_move_info(
			vehicle->var_CD,
			vehicle->track_type,
			vehicle->track_progress
			);
		x = vehicle->x + moveInfo2->x;
		y = vehicle->y + moveInfo2->y;
	}

loc_6DB8A5:
	regs.ebx = RCT2_ADDRESS(0x009A2930, uint32)[regs.ebx];
	vehicle->var_24 -= regs.ebx;
	unk_F64E20->x = x;
	unk_F64E20->y = y;
	unk_F64E20->z = z;
	vehicle->sprite_direction = moveInfo->direction;
	vehicle->var_20 = moveInfo->var_08;
	vehicle->var_1F = moveInfo->var_07;

	regs.ebx = moveInfo->var_07;

	if ((vehicleEntry->var_14 & 0x200) && moveInfo->var_07 != 0) {
		vehicle->var_4A = 0;
		vehicle->var_4C = 0;
		vehicle->var_4E = 0;
	}

	// vehicle == frontVehicle
	if (vehicle == RCT2_GLOBAL(0x00F64E00, rct_vehicle*)) {
		if (RCT2_GLOBAL(0x00F64E08, sint32) >= 0) {
			regs.bp = vehicle->prev_vehicle_on_ride;
			if (sub_6DD078(vehicle, vehicle->prev_vehicle_on_ride)) {
				goto loc_6DB967;
			}
		}
	}

loc_6DB928:
	if (vehicle->var_24 < 0x368A) {
		goto loc_6DBF20;
	}

	regs.ebx = RCT2_ADDRESS(0x009A2970, uint32)[regs.ebx];
	vehicle->var_2C += regs.ebx;
	RCT2_GLOBAL(0x00F64E10, uint32)++;
	goto loc_6DAEB9;

loc_6DB94A:
	RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_5;
	regs.eax = vehicle->var_24 + 1;
	RCT2_GLOBAL(0x00F64E0C, uint32) -= regs.eax;
	vehicle->var_24 = 0xFFFFFFFF;
	regs.ebx = vehicle->var_1F;
	goto loc_6DBE3F;

loc_6DB967:
	regs.eax = vehicle->var_24 + 1;
	RCT2_GLOBAL(0x00F64E0C, uint32) -= regs.eax;
	vehicle->var_24 -= regs.eax;

	// Might need to be bp rather than vehicle, but hopefully not
	rct_vehicle *head = vehicle_get_head(vehicle);

	regs.eax = abs(vehicle->velocity - head->velocity);
	if (!(rideEntry->flags & RIDE_ENTRY_FLAG_18)) {
		if (regs.eax > 0xE0000) {
			if (!(vehicleEntry->var_14 & (1 << 6))) {
				RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_COLLISION;
			}
		}
	}

	if (vehicleEntry->var_14 & (1 << 14)) {
		vehicle->velocity -= vehicle->velocity >> 2;
	} else {
		sint32 newHeadVelocity = vehicle->velocity >> 1;
		vehicle->velocity = head->velocity >> 1;
		head->velocity = newHeadVelocity;
	}
	RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_1;
	goto loc_6DBE3F;

loc_6DBA13:
	vehicle->var_B8 &= ~(1 << 1);
	unk_F64E20->x = vehicle->x;
	unk_F64E20->y = vehicle->y;
	unk_F64E20->z = vehicle->z;
	invalidate_sprite_2((rct_sprite*)vehicle);

loc_6DBA33:
	trackType = vehicle->track_type >> 2;
	if (trackType == TRACK_ELEM_FLAT && ride->type == RIDE_TYPE_REVERSE_FREEFALL_COASTER) {
		sint32 unkVelocity = RCT2_GLOBAL(0x00F64E08, sint32);
		if (unkVelocity > 0xFFF80000) {
			unkVelocity = abs(unkVelocity);
			vehicle->var_2C = unkVelocity * 2;
		}
	}

	if (trackType == TRACK_ELEM_BRAKES) {
		regs.eax = -(vehicle->var_CF << 16);
		if (regs.eax <= RCT2_GLOBAL(0x00F64E08, sint32)) {
			regs.eax = RCT2_GLOBAL(0x00F64E08, sint32) * -4;
			vehicle->var_2C = regs.eax;
		}
	}

	regs.ax = vehicle->track_progress - 1;
	if (regs.ax != -1) {
		goto loc_6DBD42;
	}

	RCT2_GLOBAL(0x00F64E36, uint8) = gTrackDefinitions[trackType].vangle_end;
	RCT2_GLOBAL(0x00F64E37, uint8) = gTrackDefinitions[trackType].bank_end;
	mapElement = map_get_track_element_at_of_type_seq(
		vehicle->track_x,
		vehicle->track_y,
		vehicle->track_z >> 3,
		trackType,
		0
		);

loc_6DBB08:
	x = vehicle->track_x;
	y = vehicle->track_y;
	switch (vehicle->var_CD) {
	case 3:
		vehicle->var_CD = 1;
		break;
	case 7:
		vehicle->var_CD = 5;
		break;
	case 8:
		vehicle->var_CD = 6;
		break;
	case 2:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
		vehicle->var_CD = 2;
	loc_6DBB4F:
		{
			rct_xy_element input;
			rct_xy_element output;
			int outputZ;
			int outputDirection;

			input.x = x;
			input.y = y;
			input.element = mapElement;
			if (track_block_get_next(&input, &output, &outputZ, &outputDirection)) {
				regs.edi = output.element;
				goto loc_6DBE5E;
			}
			mapElement = output.element;
			goto loc_6DBC3B;
		}
	}

loc_6DBB7E:;
	int direction;
	{
		track_begin_end trackBeginEnd;
		if (!track_block_get_previous(x, y, mapElement, &trackBeginEnd)) {
			goto loc_6DBE5E;
		}
		mapElement = trackBeginEnd.begin_element;

		trackType = mapElement->properties.track.type;
		if (trackType == 211 || trackType == 212) {
			goto loc_6DBE5E;
		}

		int trackColour = vehicle->update_flags >> 9;
		int bank = gTrackDefinitions[trackType].bank_end;
		bank = track_get_actual_bank_2(ride->type, regs.al, bank);
		int vAngle = gTrackDefinitions[trackType].vangle_end;
		if (RCT2_GLOBAL(0x00F64E36, uint8) != vAngle ||
			RCT2_GLOBAL(0x00F64E37, uint8) != bank
		) {
			goto loc_6DBE5E;
		}

		// Update VEHICLE_UPDATE_FLAG_11
		vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_11;
		if (RideData4[ride->type].flags & RIDE_TYPE_FLAG4_3) {
			if (mapElement->properties.track.colour & 4) {
				vehicle->update_flags |= VEHICLE_UPDATE_FLAG_11;
			}
		}

		x = trackBeginEnd.begin_x;
		y = trackBeginEnd.begin_y;
		z = trackBeginEnd.begin_z;
		direction = trackBeginEnd.begin_direction;
	}

loc_6DBC3B:
	vehicle->track_x = x;
	vehicle->track_y = y;
	vehicle->track_z = z;

	if (vehicle->var_CD != 0 &&
		vehicle->var_CD < 5
		) {
		sint16 xy = (x >> 5) | ((y >> 5) << 8);
		if (ride->var_13C == xy &&
			ride->var_13F == (z >> 3)
			) {
			vehicle->var_CD = 3;
		} else if (
			ride->var_13A == xy &&
			ride->var_13E == (z >> 3)
			) {
			vehicle->var_CD = 4;
		}
	}

	if (track_element_is_lift_hill(mapElement)) {
		if (RCT2_GLOBAL(0x00F64E08, sint32) < 0) {
			if (vehicle->next_vehicle_on_train == SPRITE_INDEX_NULL) {
				trackType = mapElement->properties.track.type;
				if (RCT2_ADDRESS(0x0099423C, uint16)[trackType] & 0x20) {
					RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_9;
				}
			}
			vehicle->update_flags |= VEHICLE_UPDATE_FLAG_0;
		}
	} else {
		if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_0) {
			vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_0;
			if (vehicle->next_vehicle_on_train == SPRITE_INDEX_NULL) {
				if (RCT2_GLOBAL(0x00F64E08, sint32) < 0) {
					RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_8;
				}
			}
		}
	}

	trackType = mapElement->properties.track.type;
	if (trackType != TRACK_ELEM_BRAKES) {
		vehicle->var_D9 = mapElement->properties.track.colour >> 4;
	}
	direction &= 3;
	vehicle->track_type = trackType << 2;
	vehicle->track_direction |= direction;
	vehicle->var_CF = (mapElement->properties.track.sequence >> 4) << 1;

	moveInfo = vehicle_get_move_info(
		vehicle->var_CD,
		vehicle->track_type,
		0
		);

	// There are two bytes before the move info list
	uint16 unk16 = *((uint16*)((int)moveInfo - 2));
	unk16--;
	regs.ax = unk16;

loc_6DBD42:
	vehicle->track_progress = regs.ax;
	moveInfo = vehicle_get_move_info(
		vehicle->var_CD,
		vehicle->track_type,
		0
		);

	x = vehicle->track_x + moveInfo->x;
	y = vehicle->track_y + moveInfo->y;
	z = vehicle->track_z + moveInfo->z + RCT2_GLOBAL(0x0097D21A + (ride->type * 8), uint8);

	regs.ebx = 0;
	if (x == unk_F64E20->x) { regs.ebx |= 1; }
	if (y == unk_F64E20->y) { regs.ebx |= 2; }
	if (z == unk_F64E20->z) { regs.ebx |= 4; }
	vehicle->var_24 += RCT2_ADDRESS(0x009A2930, uint32)[regs.ebx];

	unk_F64E20->x = x;
	unk_F64E20->y = y;
	unk_F64E20->z = z;
	vehicle->sprite_direction = moveInfo->direction;
	vehicle->var_20 = moveInfo->var_08;
	regs.ebx = moveInfo->var_07;
	vehicle->var_1F = regs.bl;

	if ((vehicleEntry->var_14 & 0x200) && regs.bl != 0) {
		vehicle->var_4A = 0;
		vehicle->var_4C = 0;
		vehicle->var_4E = 0;
	}

	if (vehicle == RCT2_GLOBAL(0x00F64E00, rct_vehicle*)) {
		if (RCT2_GLOBAL(0x00F64E08, sint32) >= 0) {
			regs.bp = vehicle->next_vehicle_on_ride;
			if (sub_6DD078(vehicle, regs.bp)) {
				goto loc_6DBE7F;
			}
		}
	}

loc_6DBE3F:
	if ((sint32)vehicle->var_24 >= 0) {
		goto loc_6DBF20;
	}
	regs.ebx = RCT2_ADDRESS(0x009A2970, uint32)[regs.ebx];
	vehicle->var_2C = regs.ebx;
	RCT2_GLOBAL(0x00F64E10, uint32)++;
	goto loc_6DBA33;

loc_6DBE5E:
	RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_5;
	regs.eax = (sint32)vehicle->var_24 - 0x368A;
	RCT2_GLOBAL(0x00F64E0C, uint32) -= regs.eax;
	vehicle->var_24 -= regs.eax;
	regs.ebx = vehicle->var_1F;
	goto loc_6DB928;

loc_6DBE7F:
	regs.eax = vehicle->var_24 - 0x368A;
	RCT2_GLOBAL(0x00F64E0C, uint32) -= regs.eax;
	vehicle->var_24 -= regs.eax;

	rct_vehicle *v3 = GET_VEHICLE(regs.bp);
	rct_vehicle *v4 = RCT2_GLOBAL(0x00F64E04, rct_vehicle*);
	regs.eax = abs(v4->velocity - v3->velocity);

	if (!(rideEntry->flags & RIDE_ENTRY_FLAG_18)) {
		if (regs.eax > 0xE0000) {
			if (!(vehicleEntry->var_14 & (1 << 6))) {
				RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_COLLISION;
			}
		}
	}

	if (vehicleEntry->var_14 & (1 << 14)) {
		vehicle->velocity -= vehicle->velocity >> 2;
		RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_2;
	} else {
		vehicle->velocity = v3->velocity >> 1;
		v3->velocity = v4->velocity >> 1;
		RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_2;
	}

	goto loc_6DB928;

loc_6DBF20:
	sprite_move(unk_F64E20->x, unk_F64E20->y, unk_F64E20->z, (rct_sprite*)vehicle);
	invalidate_sprite_2((rct_sprite*)vehicle);

loc_6DBF3E:
	sub_6DBF3E(vehicle);

loc_6DC0F7:
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_0) {
		RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_2;
	}
	if (RCT2_GLOBAL(0x00F64E08, sint32) >= 0) {
		if (vehicle->next_vehicle_on_train == SPRITE_INDEX_NULL) {
			goto loc_6DC144;
		}

		vehicle = GET_VEHICLE(vehicle->next_vehicle_on_train);
	} else {
		if (vehicle == RCT2_GLOBAL(0x00F64E04, rct_vehicle*)) {
			goto loc_6DC144;
		}
	}
	goto loc_6DAE27;

loc_6DC144:
	vehicle = RCT2_GLOBAL(0x00F64E04, rct_vehicle*);
	regs.eax = 0;
	regs.ebp = 0;
	regs.dx = 0;
	regs.ebx = 0;

	for (;;) {
		regs.ebx++;
		regs.dx |= vehicle->update_flags;
		regs.bp += vehicle->friction;
		regs.eax += vehicle->var_2C;

		uint16 spriteIndex = vehicle->next_vehicle_on_train;
		if (spriteIndex == SPRITE_INDEX_NULL) {
			break;
		}
		vehicle = GET_VEHICLE(spriteIndex);
	}

	vehicle = RCT2_GLOBAL(0x00F64E04, rct_vehicle*);
	regs.eax = (regs.eax / regs.ebx) * 21;
	if (regs.eax < 0) {
		regs.eax += 511;
	}
	regs.eax >>= 9;
	regs.ecx = regs.eax;
	regs.eax = vehicle->velocity;
	if (regs.eax < 0) {
		regs.eax = -regs.eax;
		regs.eax >>= 12;
		regs.eax = -regs.eax;
	} else {
		regs.eax >>= 12;
	}

	regs.ecx -= regs.eax;
	regs.edx = vehicle->velocity;
	regs.ebx = regs.edx;
	regs.edx >>= 8;
	regs.edx *= regs.edx;
	if (regs.ebx < 0) {
		regs.edx = -regs.edx;
	}
	regs.edx >>= 4;
	regs.eax = regs.edx;
	regs.eax = regs.eax / regs.ebp;
	regs.ecx -= regs.eax;

	if (!(vehicleEntry->var_14 & (1 << 3))) {
		goto loc_6DC2FA;
	}
	if (vehicleEntry->var_12 & (1 << 0)) {
		regs.eax = vehicle->speed * 0x4000;
		if (regs.eax < vehicle->velocity) {
			goto loc_6DC2FA;
		}
	}
	regs.eax = vehicle->speed;
	trackType = vehicle->track_direction >> 2;
	if (trackType == TRACK_ELEM_LEFT_QUARTER_TURN_1_TILE) {
		goto loc_6DC22F;
	}
	if (trackType != TRACK_ELEM_RIGHT_QUARTER_TURN_1_TILE) {
		goto loc_6DC23A;
	}
	if (vehicle->var_CD == 6) {
		goto loc_6DC238;
	}

loc_6DC226:
	regs.ebx = regs.eax >> 2;
	regs.eax -= regs.ebx;
	goto loc_6DC23A;

loc_6DC22F:
	if (vehicle->var_CD != 5) {
		goto loc_6DC226;
	}

loc_6DC238:
	regs.eax >>= 1;

loc_6DC23A:
	regs.ebx = regs.eax;
	regs.eax <<= 14;
	regs.ebx *= regs.ebp;
	regs.ebx >>= 2;
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_3) {
		regs.eax = -regs.eax;
	}
	regs.eax -= vehicle->velocity;
	regs.edx = vehicle->acceleration;
	regs.edx <<= 1;
	regs.eax *= regs.edx;
	regs.eax /= regs.ebx;

	if (vehicleEntry->var_12 & (1 << 15)) {
		regs.eax <<= 2;
	}

	if (!(vehicleEntry->var_14 & (1 << 13))) {
		goto loc_6DC2E3;
	}

	if (regs.eax < 0) {
		regs.eax >>= 4;
	}

	if (vehicleEntry->var_14 & (1 << 2)) {
		regs.bx = vehicle->var_B6;
		if (regs.bx > 512) {
			regs.bx = 512;
		}
		if (regs.bx < -512) {
			regs.bx = -512;
		}
		vehicle->var_B6 = regs.bx;
	}

	if (vehicle->var_1F != 0) {
		if (regs.eax < 0) {
			regs.eax = 0;
		}

		if (!(vehicleEntry->var_14 & (1 << 2))) {
			if (vehicle->var_1F == 2) {
				vehicle->var_B6 = 0;
			}
		}
		goto loc_6DC2F6;
	}

loc_6DC2E3:
	regs.ebx = vehicle->velocity;
	if (regs.ebx < 0) {
		regs.ebx = -regs.ebx;
	}
	if (regs.ebx <= 0x10000) {
		regs.ecx = 0;
	}

loc_6DC2F6:
	regs.ecx += regs.eax;
	goto loc_6DC316;

loc_6DC2FA:
	if (regs.ecx <= 0) {
		if (regs.ecx >= -500) {
			if (vehicle->velocity <= 0x8000) {
				regs.ecx += 400;
			}
		}
	}

loc_6DC316:
	regs.bx = vehicle->track_type >> 2;
	if (regs.bx == TRACK_ELEM_WATER_SPLASH) {
		if (vehicle->track_progress >= 48 &&
			vehicle->track_progress <= 128
			) {
			regs.eax = vehicle->velocity >> 6;
			regs.ecx -= regs.eax;
		}
	}

	if (rideEntry->flags & RIDE_ENTRY_FLAG_9) {
		if (!vehicle->is_child) {
			regs.bx = vehicle->track_type >> 2;
			if (track_element_is_covered(regs.bx)) {
				if (vehicle->velocity > 0x20000) {
					regs.eax = vehicle->velocity >> 6;
					regs.ecx -= regs.eax;
				}
			}
		}
	}

	vehicle->var_2C = regs.ecx;

	regs.eax = RCT2_GLOBAL(0x00F64E18, uint32);
	regs.ebx = RCT2_GLOBAL(0x00F64E1C, uint32);
	if (ride->lifecycle_flags & RIDE_LIFECYCLE_SIX_FLAGS_DEPRECATED) {
		regs.eax &= ~VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_DERAILED;
		regs.eax &= ~VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_COLLISION;
	}
	goto end;

loc_6DC3A7:
	RCT2_GLOBAL(0x00F64E04, rct_vehicle*) = vehicle;
	RCT2_GLOBAL(0x00F64E18, uint32) = 0;
	vehicle->velocity += vehicle->var_2C;
	RCT2_GLOBAL(0x00F64E08, sint32) = vehicle->velocity;
	RCT2_GLOBAL(0x00F64E0C, sint32) = (vehicle->velocity >> 10) * 42;
	if (RCT2_GLOBAL(0x00F64E08, sint32) < 0) {
		vehicle = vehicle_get_tail(vehicle);
	}
	RCT2_GLOBAL(0x00F64E00, rct_vehicle*) = vehicle;

loc_6DC40E:
	regs.ebx = vehicle->var_1F;
	RCT2_GLOBAL(0x00F64E10, uint32) = 1;
	vehicle->var_2C = dword_9A2970[vehicle->var_1F];
	vehicle->var_24 = RCT2_GLOBAL(0x00F64E0C, uint32) + vehicle->var_24;
	if ((sint32)vehicle->var_24 < 0) {
		goto loc_6DCA7A;
	}
	if ((sint32)vehicle->var_24 < 0x368A) {
		goto loc_6DCE02;
	}
	vehicle->var_B8 &= ~(1 << 1);
	unk_F64E20->x = vehicle->x;
	unk_F64E20->y = vehicle->y;
	unk_F64E20->z = vehicle->z;
	invalidate_sprite_2((rct_sprite*)vehicle);

loc_6DC462:
	vehicle->var_D3 = 0;
	if (vehicle->var_D3 == 0) {
		goto loc_6DC476;
	}
	vehicle->var_D3--;
	goto loc_6DC985;

loc_6DC476:
	if (!(vehicle->var_D5 & (1 << 2))) {
		regs.edi = RCT2_ADDRESS(0x008B8F74, uint32)[vehicle->var_D4];
		regs.al = vehicle->var_C5 + 1;
		if ((uint8)regs.al < ((uint8*)regs.edi)[-1]) {
			vehicle->var_C5 = regs.al;
			goto loc_6DC985;
		}
		vehicle->var_D5 &= ~(1 << 2);
	}

	if (vehicle->var_D5 & (1 << 0)) {
		regs.di = vehicle->is_child ? vehicle->prev_vehicle_on_ride : vehicle->next_vehicle_on_ride;
		rct_vehicle *vEDI = GET_VEHICLE(regs.di);
		if (!(vEDI->var_D5 & (1 << 0)) || (vEDI->var_D5 & (1 << 2))) {
			goto loc_6DC985;
		}
		if (vEDI->var_D3 != 0) {
			goto loc_6DC985;
		}
		vEDI->var_D5 &= ~(1 << 0);
		vehicle->var_D5 &= ~(1 << 0);
	}

	if (vehicle->var_D5 & (1 << 1)) {
		regs.di = vehicle->is_child ? vehicle->prev_vehicle_on_ride : vehicle->next_vehicle_on_ride;
		rct_vehicle *vEDI = GET_VEHICLE(regs.di);
		if (!(vEDI->var_D5 & (1 << 1)) || (vEDI->var_D5 & (1 << 2))) {
			goto loc_6DC985;
		}
		if (vEDI->var_D3 != 0) {
			goto loc_6DC985;
		}
		vEDI->var_D5 &= ~(1 << 1);
		vehicle->var_D5 &= ~(1 << 1);
	}

	if (vehicle->var_D5 & (1 << 3)) {
		rct_vehicle *vEDI = vehicle;

		for (;;) {
			vEDI = GET_VEHICLE(vEDI->prev_vehicle_on_ride);
			if (vEDI == vehicle) {
				break;
			}
			if (!vEDI->is_child) continue;
			if (!(vEDI->var_D5 & (1 << 4))) continue;
			if (vEDI->track_x != vehicle->track_x) continue;
			if (vEDI->track_y != vehicle->track_y) continue;
			if (vEDI->track_z != vehicle->track_z) continue;
			goto loc_6DC985;
		}

		vehicle->var_D5 |= (1 << 4);
		vehicle->var_D5 &= ~(1 << 3);
	}

loc_6DC5B8:
	moveInfo = vehicle_get_move_info(vehicle->var_CD, vehicle->track_type, 0);

	// There are two bytes before the move info list
	{
		uint16 unk16_v34 = vehicle->track_progress + 1;
		uint16 unk16 = *((uint16*)((int)moveInfo - 2));
		if (unk16_v34 < unk16) {
			regs.ax = unk16_v34;
			goto loc_6DC743;
		}
	}

	trackType = vehicle->track_type >> 2;
	RCT2_GLOBAL(0x00F64E36, uint8) = gTrackDefinitions[trackType].vangle_end;
	RCT2_GLOBAL(0x00F64E37, uint8) = gTrackDefinitions[trackType].bank_end;
	mapElement = map_get_track_element_at_of_type_seq(
		vehicle->track_x, vehicle->track_y, vehicle->track_z,
		trackType, 0
	);
	{
		rct_xy_element input, output;
		int outZ, outDirection;
		input.x = vehicle->track_x;
		input.y = vehicle->track_y;
		input.element = mapElement;
		if (!track_block_get_next(&input, &output, &outZ, &outDirection)) {
			goto loc_6DC9BC;
		}
		mapElement = output.element;
		x = output.x;
		y = output.y;
		z = outZ;
		direction = outDirection;
	}

	if (!loc_6DB38B(vehicle, mapElement)) {
		goto loc_6DC9BC;
	}

	int rideType = GET_RIDE(mapElement->properties.track.ride_index)->type;
	vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_11;
	if (RideData4[rideType].flags & RIDE_TYPE_FLAG4_3) {
		if (mapElement->properties.track.colour & (1 << 2)) {
			vehicle->update_flags |= VEHICLE_UPDATE_FLAG_11;
		}
	}

	vehicle->track_x = x;
	vehicle->track_y = y;
	vehicle->track_z = z;

	if (vehicle->is_child) {
		rct_vehicle *prevVehicle = GET_VEHICLE(vehicle->prev_vehicle_on_ride);
		regs.al = prevVehicle->var_CD;
		if (regs.al != 0) {
			regs.al--;
		}
		vehicle->var_CD = regs.al;
	}

	vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_0;
	vehicle->track_type = (mapElement->properties.track.type << 2) | (direction & 3);
	vehicle->var_CF = (mapElement->properties.track.sequence >> 4) << 1;
	regs.ax = 0;

loc_6DC743:
	vehicle->track_progress = regs.ax;
	if (vehicle->is_child) {
		vehicle->var_C5++;
		if (vehicle->var_C5 >= 6) {
			vehicle->var_C5 = 0;
		}
	}

	for (;;) {
		moveInfo = vehicle_get_move_info(vehicle->var_CD, vehicle->track_type, vehicle->track_progress);
		if (moveInfo->x != (uint16)0x8000) {
			break;
		}
		switch (moveInfo->y) {
		case 0: // loc_6DC7B4
			if (vehicle->is_child) {
				vehicle->var_D5 |= (1 << 3);
			} else {
				uint16 rand16 = scenario_rand() & 0xFFFF;
				regs.bl = 14;
				if (rand16 <= 0xA000) {
					regs.bl = 12;
					if (rand16 <= 0x900) {
						regs.bl = 10;
					}
				}
				vehicle->var_CD = regs.bl;
			}
			vehicle->track_progress++;
			break;
		case 1: // loc_6DC7ED
			vehicle->var_D3 = moveInfo->z;
			vehicle->track_progress++;
			break;
		case 2: // loc_6DC800
			vehicle->var_D5 |= (1 << 0);
			vehicle->track_progress++;
			break;
		case 3: // loc_6DC810
			vehicle->var_D5 |= (1 << 1);
			vehicle->track_progress++;
			break;
		case 4: // loc_6DC820
			z = moveInfo->z;
			if (z == 2) {
				rct_peep *peep = GET_PEEP(vehicle->peep[0]);
				if (peep->id & 7) {
					z = 7;
				}
			}
			if (trackType == 6) {
				rct_peep *peep = GET_PEEP(vehicle->peep[0]);
				if (peep->id & 7) {
					z = 8;
				}
			}
			vehicle->var_D4 = z;
			vehicle->var_C5 = 0;
			vehicle->track_progress++;
			break;
		case 5: // loc_6DC87A
			vehicle->var_D5 |= (1 << 2);
			vehicle->track_progress++;
			break;
		case 6: // loc_6DC88A
			vehicle->var_D5 &= ~(1 << 4);
			vehicle->var_D5 |= (1 << 5);
			vehicle->track_progress++;
			break;
		default:
			log_error("Invalid move info...");
			assert(false);
			break;
		}
	}

loc_6DC8A1:
	x = vehicle->track_x + moveInfo->x;
	y = vehicle->track_y + moveInfo->y;
	z = vehicle->track_z + moveInfo->z + RCT2_GLOBAL(0x0097D21A + (ride->type * 8), uint8);

	// Investigate redundant code
	regs.ebx = 0;
	if (regs.ax != unk_F64E20->x) {
		regs.ebx |= 1;
	}
	if (regs.cx == unk_F64E20->y) {
		regs.ebx |= 2;
	}
	if (regs.dx == unk_F64E20->z) {
		regs.ebx |= 4;
	}
	regs.ebx = 0x368A;
	vehicle->var_24 -= regs.ebx;
	if ((sint32)vehicle->var_24 < 0) {
		vehicle->var_24 = 0;
	}

	unk_F64E20->x = x;
	unk_F64E20->y = y;
	unk_F64E20->z = z;
	vehicle->sprite_direction = moveInfo->direction;
	vehicle->var_20 = moveInfo->var_08;
	vehicle->var_1F = moveInfo->var_07;

	if (rideEntry->vehicles[0].var_14 & (1 << 9)) {
		if (vehicle->var_1F != 0) {
			vehicle->var_4A = 0;
			vehicle->var_4C = 0;
			vehicle->var_4E = 0;
		}
	}

	if (vehicle == RCT2_GLOBAL(0x00F64E00, rct_vehicle*)) {
		if (RCT2_GLOBAL(0x00F64E08, sint32) >= 0) {
			sub_6DD078(vehicle, vehicle->var_44);
		}
	}
	goto loc_6DC99A;

loc_6DC985:
	regs.ebx = 0;
	vehicle->var_24 -= 0x368A;
	if ((sint32)vehicle->var_24 < 0) {
		vehicle->var_24 = 0;
	}

loc_6DC99A:
	if ((sint32)vehicle->var_24 < 0x368A) {
		goto loc_6DCDE4;
	}
	vehicle->var_2C = dword_9A2970[vehicle->var_1F];
	RCT2_GLOBAL(0x00F64E10, uint32)++;
	goto loc_6DC462;

loc_6DC9BC:
	RCT2_GLOBAL(0x00F64E18, uint32) |= 0x20;
	regs.eax = vehicle->var_24 + 1;
	RCT2_GLOBAL(0x00F64E0C, uint32) -= regs.eax;
	vehicle->var_24 -= regs.eax;
	regs.ebx = vehicle->var_1F;
	goto loc_6DCD2B;

	/////////////////////////////////////////
	// Dead code: 0x006DC9D9 to 0x006DCA79 //
	/////////////////////////////////////////

loc_6DCA7A:
	vehicle->var_B8 &= ~(1 << 1);
	unk_F64E20->x = vehicle->x;
	unk_F64E20->y = vehicle->y;
	unk_F64E20->z = vehicle->z;
	invalidate_sprite_2((rct_sprite*)vehicle);

loc_6DCA9A:
	regs.ax = vehicle->track_progress - 1;
	if (regs.ax != (short)0xFFFF) {
		goto loc_6DCC2C;
	}
	
	trackType = vehicle->track_type >> 2;
	RCT2_GLOBAL(0x00F64E36, uint8) = gTrackDefinitions[trackType].vangle_end;
	RCT2_GLOBAL(0x00F64E37, uint8) = gTrackDefinitions[trackType].bank_end;
	mapElement = map_get_track_element_at_of_type_seq(
		vehicle->track_x, vehicle->track_y, vehicle->track_z,
		trackType, 0
	);
	{
		track_begin_end trackBeginEnd;
		if (!track_block_get_previous(vehicle->track_x, vehicle->track_y, mapElement, &trackBeginEnd)) {
			goto loc_6DC9BC;
		}
		x = trackBeginEnd.begin_x;
		y = trackBeginEnd.begin_y;
		z = trackBeginEnd.begin_z;
		direction = trackBeginEnd.begin_direction;
		mapElement = trackBeginEnd.begin_element;
	}

	if (!loc_6DB38B(vehicle, mapElement)) {
		goto loc_6DCD4A;
	}

	rideType = GET_RIDE(mapElement->properties.track.ride_index)->type;
	vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_11;
	if (RideData4[rideType].flags & RIDE_TYPE_FLAG4_3) {
		if (mapElement->properties.track.colour & (1 << 2)) {
			vehicle->update_flags |= VEHICLE_UPDATE_FLAG_11;
		}
	}

	vehicle->track_x = x;
	vehicle->track_y = y;
	vehicle->track_z = z;

	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_0) {
		vehicle->update_flags &= ~VEHICLE_UPDATE_FLAG_0;
		if (vehicle->next_vehicle_on_train == SPRITE_INDEX_NULL) {
			if (RCT2_GLOBAL(0x00F64E08, sint32) < 0) {
				RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_8;
			}
		}
	}

	vehicle->track_type = (mapElement->properties.track.type << 2) | (direction & 3);
	vehicle->var_CF = (mapElement->properties.track.colour >> 4) << 1;
	
	moveInfo = vehicle_get_move_info(vehicle->var_CD, vehicle->track_type, 0);
	
	// There are two bytes before the move info list
	regs.ax = *((uint16*)((int)moveInfo - 2)) - 1;

loc_6DCC2C:
	vehicle->track_progress = regs.ax;

	moveInfo = vehicle_get_move_info(vehicle->var_CD, vehicle->track_type, vehicle->track_progress);
	x = vehicle->track_x + moveInfo->x;
	y = vehicle->track_y + moveInfo->y;
	z = vehicle->track_z + moveInfo->z + RCT2_GLOBAL(0x0097D21A + (ride->type * 8), uint8);

	// Investigate redundant code
	regs.ebx = 0;
	if (regs.ax != unk_F64E20->x) {
		regs.ebx |= 1;
	}
	if (regs.cx == unk_F64E20->y) {
		regs.ebx |= 2;
	}
	if (regs.dx == unk_F64E20->z) {
		regs.ebx |= 4;
	}
	regs.ebx = 0x368A;
	vehicle->var_24 -= regs.ebx;
	if ((sint32)vehicle->var_24 < 0) {
		vehicle->var_24 = 0;
	}

	unk_F64E20->x = x;
	unk_F64E20->y = y;
	unk_F64E20->z = z;
	vehicle->sprite_direction = moveInfo->direction;
	vehicle->var_20 = moveInfo->var_08;
	vehicle->var_1F = moveInfo->var_07;

	if (rideEntry->vehicles[0].var_14 & (1 << 9)) {
		if (vehicle->var_1F != 0) {
			vehicle->var_4A = 0;
			vehicle->var_4C = 0;
			vehicle->var_4E = 0;
		}
	}

	if (vehicle == RCT2_GLOBAL(0x00F64E00, rct_vehicle*)) {
		if (RCT2_GLOBAL(0x00F64E08, sint32) >= 0) {
			if (sub_6DD078(vehicle, vehicle->var_44)) {
				regs.bp = vehicle->var_44;
				goto loc_6DCD6B;
			}
		}
	}

loc_6DCD2B:
	if (vehicle->var_24 >= 0) {
		goto loc_6DCDE4;
	}
	vehicle->var_2C += RCT2_ADDRESS(0x009A2970, uint32)[vehicle->var_1F];
	RCT2_GLOBAL(0x00F64E10, uint32)++;
	goto loc_6DCA9A;

loc_6DCD4A:
	RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_5;
	regs.eax = vehicle->var_24 - 0x368A;
	RCT2_GLOBAL(0x00F64E0C, uint32) -= regs.eax;
	vehicle->var_24 -= regs.eax;
	regs.ebx = vehicle->var_1F;
	goto loc_6DC99A;

loc_6DCD6B:
	regs.eax = vehicle->var_24 - 0x368A;
	RCT2_GLOBAL(0x00F64E0C, uint32) -= regs.eax;
	vehicle->var_24 -= regs.eax;
	rct_vehicle *vEBP = GET_VEHICLE(regs.bp);
	rct_vehicle *vEDI = RCT2_GLOBAL(0x00F64E04, uint32);
	regs.eax = abs(vEDI->velocity - vEBP->velocity);
	if (regs.eax > 0xE0000) {
		if (!(vehicleEntry->var_14 & (1 << 6))) {
			RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_COLLISION;
		}
	}
	vEDI->velocity = vEBP->velocity >> 1;
	vEBP->velocity = vEDI->velocity >> 1;
	RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_2;
	goto loc_6DC99A;

loc_6DCDE4:
	sprite_move(unk_F64E20->x, unk_F64E20->y, unk_F64E20->z, (rct_sprite*)vehicle);
	invalidate_sprite_2((rct_sprite*)vehicle);

loc_6DCE02:
	vehicle->var_2C /= RCT2_GLOBAL(0x00F64E10, uint32);
	if (vehicle->var_CD == 2) {
		goto loc_6DCEB2;
	}
	trackType = vehicle->track_type >> 2;
	if (!(RCT2_GLOBAL(0x0099BA64 + (trackType * 16), uint32) & 0x10)) {
		goto loc_6DCEB2;
	}
	RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_3;
	if (trackType != TRACK_ELEM_END_STATION) {
		goto loc_6DCEB2;
	}
	if (vehicle != RCT2_GLOBAL(0x00F64E04, rct_vehicle*)) {
		goto loc_6DCEB2;
	}
	regs.ax = vehicle->track_progress;
	if (RCT2_GLOBAL(0x00F64E08, sint32) < 0) {
		goto loc_6DCE62;
	}
	regs.cx = 8;
	if (regs.ax > regs.cx) {
		goto loc_6DCE68;
	}
	goto loc_6DCEB2;

loc_6DCE62:
	if (regs.ax > 11) {
		goto loc_6DCEB2;
	}

loc_6DCE68:
	RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_VEHICLE_AT_STATION;
	regs.al = vehicle->track_x >> 5;
	regs.ah = vehicle->track_y >> 5;
	regs.dl = vehicle->track_z >> 3;
	for (int i = 0; i < 4; i++) {
		if ((uint16)regs.ax != ride->station_starts[i]) {
			continue;
		}
		if ((uint16)regs.dl != ride->station_heights[i]) {
			continue;
		}
		RCT2_GLOBAL(0x00F64E1C, uint32) = i;
	}

loc_6DCEB2:
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_0) {
		RCT2_GLOBAL(0x00F64E18, uint32) |= VEHICLE_UPDATE_MOTION_TRACK_FLAG_4;
	}
	if (RCT2_GLOBAL(0x00F64E08, sint32) >= 0) {
		regs.si = vehicle->next_vehicle_on_train;
		if ((uint16)regs.si == SPRITE_INDEX_NULL) {
			goto loc_6DCEFF;
		}
		vehicle = GET_VEHICLE((uint16)regs.si);
		goto loc_6DC40E;
	}

	if (vehicle == RCT2_GLOBAL(0x00F64E04, rct_vehicle*)) {
		goto loc_6DCEFF;
	}
	vehicle = GET_VEHICLE(vehicle->prev_vehicle_on_ride);
	goto loc_6DC40E;

loc_6DCEFF:
	vehicle = RCT2_GLOBAL(0x00F64E04, rct_vehicle*);
	regs.eax = 0;
	regs.ebp = 0;
	regs.dx = 0;
	regs.ebx = 0;

	for (;;) {
		regs.ebx++;
		regs.dx |= vehicle->update_flags;
		regs.bp += vehicle->friction;
		regs.eax += vehicle->var_2C;
		regs.si = vehicle->next_vehicle_on_train;
		if ((uint16)regs.si == SPRITE_INDEX_NULL) {
			break;
		}
		vehicle = GET_VEHICLE((uint16)regs.si);
	}

	vehicle = RCT2_GLOBAL(0x00F64E04, rct_vehicle*);
	regs.ebx /= regs.eax;
	regs.ecx = (regs.eax * 21) >> 9;
	regs.eax = vehicle->velocity >> 12;
	regs.ecx -= regs.eax;
	regs.ebx = vehicle->velocity;
	regs.edx = vehicle->velocity >> 8;
	regs.edx *= regs.edx;
	if (regs.ebx < 0) {
		regs.edx = -regs.edx;
	}
	regs.edx >>= 4;
	regs.eax = regs.edx / regs.ebp;
	regs.ecx -= regs.eax;
	
	if (!(vehicleEntry->var_14 & (1 << 3))) {
		goto loc_6DD069;
	}
	if (vehicleEntry->var_12 & (1 << 0)) {
		regs.eax = vehicle->speed * 0x4000;
		if (regs.eax < vehicle->velocity) {
			goto loc_6DD069;
		}
	}
	regs.eax = vehicle->speed;
	regs.bx = vehicle->track_type >> 2;
	regs.ebx = regs.eax;
	regs.eax <<= 14;
	regs.ebx *= regs.ebp;
	regs.ebx >>= 2;
	if (vehicle->update_flags & VEHICLE_UPDATE_FLAG_3) {
		regs.eax = -regs.eax;
	}
	regs.eax -= vehicle->velocity;
	regs.edx = vehicle->acceleration;
	regs.edx <<= 1;
	regs.eax *= regs.edx;
	regs.eax = regs.eax / regs.ebx;
	
	if (!(vehicleEntry->var_14 & (1 << 13))) {
		goto loc_6DD054;
	}

	if (regs.eax < 0) {
		regs.eax >>= 4;
	}

	if (vehicleEntry->var_14 & (1 << 2)) {
		vehicle->var_B6 = clamp(-0x200, (sint16)vehicle->var_B6, 0x200);
	}

	if (vehicle->var_1F != 0) {
		regs.eax = max(0, regs.eax);
		if (vehicleEntry->var_14 & (1 << 2)) {
			if (vehicle->var_1F == 2) {
				vehicle->var_B6 = 0;
			}
		}
	} else {
	loc_6DD054:
		regs.ebx = abs(vehicle->velocity);
		if (regs.ebx > 0x10000) {
			regs.ecx = 0;
		}
	}
	regs.ecx += regs.eax;

loc_6DD069:
	vehicle->var_2C = regs.ecx;
	regs.eax = RCT2_GLOBAL(0x00F64E18, uint32);
	regs.ebx = RCT2_GLOBAL(0x00F64E1C, uint32);

end:
	// hook_setreturnregisters(&regs);
	if (outStation != NULL) *outStation = regs.ebx;
	return regs.eax;
}

rct_ride_type_vehicle *vehicle_get_vehicle_entry(rct_vehicle *vehicle)
{
	rct_ride_type *rideEntry = GET_RIDE_ENTRY(vehicle->ride_subtype);
	return &rideEntry->vehicles[vehicle->vehicle_type];
}

int vehicle_get_total_num_peeps(rct_vehicle *vehicle)
{
	uint16 spriteIndex;
	int numPeeps = 0;
	for (;;) {
		numPeeps += vehicle->num_peeps;
		spriteIndex = vehicle->next_vehicle_on_train;
		if (spriteIndex == SPRITE_INDEX_NULL)
			break;

		vehicle = &(g_sprite_list[spriteIndex].vehicle);
	}

	return numPeeps;
}

/**
 *
 *  rct2: 0x006DA1EC
 */
void vehicle_invalidate_window(rct_vehicle *vehicle)
{
	int viewVehicleIndex;
	rct_ride *ride;
	rct_window *w;

	w = window_find_by_number(WC_RIDE, vehicle->ride);
	if (w == NULL)
		return;

	ride = GET_RIDE(vehicle->ride);
	viewVehicleIndex = w->ride.view - 1;
	if (viewVehicleIndex < 0 || viewVehicleIndex >= ride->num_vehicles)
		return;

	if (vehicle->sprite_index != ride->vehicles[viewVehicleIndex])
		return;

	window_invalidate(w);
}
