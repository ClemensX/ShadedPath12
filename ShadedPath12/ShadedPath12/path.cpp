#include "stdafx.h"

XMFLOAT3 titlePos[] = {
	XMFLOAT3(338.0f, 218.0f, -158.0f),
	XMFLOAT3(374.0f, 244.0f, -427.0f),
	XMFLOAT3(848.0f, 215.0f, -497.0f),
	XMFLOAT3(873.33f, 346.44f, -875.13f),
	XMFLOAT3(960.0f, 381.0f, -1020.0f)
	//D3DXVECTOR3(962.0f, 384.0f, -1026.0f)
};
XMFLOAT3 camIntroPos[] = {
	XMFLOAT3(455.79f, 259.71f, -438.67f),
	XMFLOAT3(495.92f, 231.05f, -711.87f),
	XMFLOAT3(972.0f, 388.0f, -1037.0f)
};

/*struct PathDesc {
	float speed;
    D3DXVECTOR3 *pos;
    D3DXVECTOR3 *look;
	int curSegment;
	int numSegments;
	LONGLONG starttime;
	D3DXVECTOR3 currentPos;
};*/


// overall speed factor for path movements:
float SpeedFactor = 1.0f;	// normal speed
//float SpeedFactor = 0.2f;	// slow demo speed
//float SpeedFactor = 4.0f;	// ultra fast, to get to endpoint quickly

PathDesc paths[] = {
	//{ 2.0f*SpeedFactor },
	//{}
	{ 2.0f*SpeedFactor, &titlePos[0], NULL, 0, 4, 0L },
	{ 1.5f*SpeedFactor, &camIntroPos[0], NULL, 0, 2, 0L }
//	{0.4f, &titlePos[0], NULL, 0, 4, 0L},
//	{0.3f, &camIntroPos[0], NULL, 0, 2, 0L}
};

//XMMATRIX interpolationMatrices[100];
//XMMATRIX interpolationMatricesChained[100];  // all children already premultiplied by parent

void Path::addRandomNPC(WorldObject *wo, char *name) {
	if (npcPaths.count(name) == 0) {
		createNpcPath(name);
	}
	PathDesc *pd = new PathDesc(this->npcPaths[name]);
	wo->pathDescMove = pd;
}

PathDesc *Path::createNpcPath(char *name) {
	PathDesc *p;
	if (std::string(name).compare("WORM") == 0) {
		p = &npcPaths[name];
		p->pathMode = Path_Random;
		p->starttime = 0;
		p->segments = 0;
		p->speed = 0.3f;
		p->animState = Anim_Stopped;
		p->minturn = XM_PI / -4.0f; // -90 degrees
		p->maxturn = p->minturn * -1.0f;
		// turn speed in radians per second:
		p->turnspeed = XM_PIDIV2; // half circle in 2 s
		p->minPathLen = 20.0f;
		p->maxPathLen = 50.0f;
		return p;
	}
	assert (false); // unknnown npc pathing type
	return nullptr;
}

void Path::moveNpc(WorldObject *wo, LONGLONG now, LONGLONG ticks_per_second, Terrain *terrain) {
	PathDesc *pd = wo->pathDescMove;
	assert(pd->pathMode == Path_Random);
	if (pd->starttime == 0) {
		// currently not moving, start this NPC:
		pd->starttime = now;
		pd->animState = Anim_Stopped;
	}
	if (pd->animState == Anim_Stopped) {
		// random turn radians:
		float turn = MathHelper::RandF(pd->minturn, pd->maxturn);
		// ticks needed for this turn:
		double turn_seconds = abs(turn) / pd->turnspeed;
		double nt = turn_seconds * (double)ticks_per_second;
		pd->num_ticks = (LONGLONG)nt;//turn_seconds * ticks_per_second; // turn time in game ticks
		pd->yawSourceAngle = wo->rot().x;
		pd->yawTargetAngle = wo->rot().x + turn;
		pd->animState = Anim_Turn;
	} else if (pd->animState == Anim_Turn) {
		LONGLONG passed = now - pd->starttime;
		if (passed > pd->num_ticks) {
			// turn completed
			if (pd->yawTargetAngle < 0) {
				wo->rot().x = XM_PI * 2.0f + pd->yawTargetAngle;
			} else {
				wo->rot().x = pd->yawTargetAngle;
			}
			while (wo->rot().x > XM_PI * 2.0) {
				wo->rot().x -= XM_PI * 2.0;
			}
			pd->animState = Anim_Move;
			pd->starttime = -1;  // signal start of move
		} else {
			// turning
			float percentage = ((float) passed) / ((float)pd->num_ticks);
			float cur = pd->yawSourceAngle + ((pd->yawTargetAngle - pd->yawSourceAngle) * percentage);
			if (cur < 0) {
				cur = XM_PI * 2.0f + cur;
			}
			wo->rot().x = cur;
			//std::ostringstream oss;
			//oss << " percentage " << percentage << " cur " << cur << "\n";
			//Blender::Log(oss.str());
		}
	} else if (pd->animState == Anim_Move) {
		if (pd->starttime == -1) {
			// begin new move along current lookAt
			float pathLen = MathHelper::RandF(pd->minPathLen, pd->maxPathLen);
			pd->starttime = now;
			pd->sourcePos = XMFLOAT3(wo->pos().x, wo->pos().y, wo->pos().z);
			// get rotation matrix for current yaw:
			XMMATRIX rot = XMMatrixRotationRollPitchYaw(0.0f, wo->rot().x, 0.0f);
			// rotate z axis 
			static XMFLOAT3 z(0.0f, 0.0f, 1.0f);
			XMVECTOR t = XMVector3Transform(XMLoadFloat3(&z), rot);
			t = XMVector3Normalize(t);
			XMVECTOR l = XMVector3Length(t);
			XMVECTOR src = XMLoadFloat3(&pd->sourcePos);
			if (isSlopeForPathOK(1.0f, &src, &t, pathLen, terrain)) {
				XMStoreFloat3(&pd->targetPos, src + t * pathLen);
			} else {
				// path leads over too steep terrain: go into stop mode so that turn is next
				pd->starttime = now;
				pd->animState = Anim_Stopped;
				return;
			}
			//XMFLOAT3 move;
			//XMStoreFloat3(&move, t*pathLen);
			//std::ostringstream oss;
			//oss << " length " << XMVectorGetX(l) << " angle " << wo->rot().x << "\n";
			//oss << " move " << move.x << "," << move.y << "," << move.z << "\n";
			//Blender::Log(oss.str());
		} else {
			// continue move
			XMVECTOR start = XMLoadFloat3(&pd->sourcePos);
			XMVECTOR end = XMLoadFloat3(&pd->targetPos);
			XMVECTOR segment = end-start;
			XMVECTOR len = XMVector3Length(segment);
			float num_ticks = XMVectorGetX(len) / pd->speed * 100000;
			LONGLONG passed = now - pd->starttime;

			if (passed > num_ticks) {
				// advanced past next segment start
				pd->starttime = now;
				pd->animState = Anim_Stopped;
				return;
			}
			XMVECTOR cur = start + (segment * (passed / num_ticks));
			wo->pos().x = XMVectorGetX(cur);
			wo->pos().z = XMVectorGetZ(cur);
//			wo->pos().y = terrain->GetHeight(wo->pos().x, wo->pos().z);
		}
	}
}

bool Path::isSlopeForPathOK(float maxSlope, XMVECTOR *start, XMVECTOR *normalizedToVector, float pathLen, Terrain *terrain) {
/*	// immediately return false if target position is on too much slope
	XMVECTOR target = *start + *normalizedToVector * pathLen;
	if (terrain->GetSlope(XMVectorGetX(target), XMVectorGetZ(target)) > maxSlope)
		return false;
	// now check every cell between start and end position for correct slope
	int checkpoints = pathLen / terrain->GetCellSpacing();
	for ( int i = 0; i < checkpoints; i++) {
		target = *start + *normalizedToVector * (pathLen/(i*terrain->GetCellSpacing()));
		if (terrain->GetSlope(XMVectorGetX(target), XMVectorGetZ(target)) > maxSlope)
			return false;
	}
*/	return true;
}

XMFLOAT3& Path::getPos(int pathID, LONGLONG now, LONGLONG ticks_per_second) {
	PathDesc *pd = &paths[pathID];
	if (pd->curSegment == -1 || pd->curSegment >= pd->numSegments)
	    return pd->pos[pd->numSegments];
	
	if (pd->starttime == 0L) {
        pd->starttime = now;
		return pd->pos[pd->curSegment];
	}

	XMVECTOR p0 = XMLoadFloat3(&pd->pos[pd->curSegment]);
	XMVECTOR p1 = XMLoadFloat3(&pd->pos[pd->curSegment + 1]);
	XMVECTOR segment = p1 - p0;
	float len = XMVectorGetX(XMVector3Length(segment));
	float num_ticks = len / pd->speed * 100000;
	LONGLONG passed = now - pd->starttime;

	if (passed > num_ticks) {
		// advanced past next segment start
		pd->curSegment++;
		if (pd->curSegment >= pd->numSegments)
			pd->curSegment = -1;
		pd->starttime = now-100;
		return getPos(pathID, now, (LONGLONG)num_ticks);
	}

	/*
	WCHAR infoLine[256];
	WCHAR* pInfoLine = infoLine;
	StringCchPrintf(pInfoLine, 256, L"p0 = (%.2f,%.2f,%.2f) p1 = (%.2f,%.2f,%.2f) scale %.2f %I64d\n",
		p0.x, p0.y, p0.z, p1.x, p1.y, p1.z, passed/num_ticks, now);
	OutputDebugString(pInfoLine);
	*/
	if (passed / num_ticks < 0) {
		throw "wrong!!!!"; // should never happen
	}
	XMVECTOR currentPos = XMVectorScale(segment, passed / num_ticks);
	currentPos = p0 + currentPos;
	XMStoreFloat3(&pd->currentPos, currentPos);
	return pd->currentPos;
}

UINT getFramePosOfSegmentStart(WorldObject &o, UINT n, PathDesc *pd) {
	BezTriple bz;
	if (pd->isBoneAnimation) {
		bz = o.boneAction->curves[0].bezTriples[n];
	} else {
		bz = o.action->curves[0].bezTriples[n];
	}
	return (UINT)bz.cp[0];
}

UINT getFrameLenghOfSegment(WorldObject &o, UINT n, PathDesc *pd) {
	return getFramePosOfSegmentStart(o, n+1, pd) - getFramePosOfSegmentStart(o, n, pd);
}

void fillPosVector(WorldObject &o, UINT n, XMFLOAT3& vec) {
	BezTriple bz = o.action->curves[0].bezTriples[n];
	vec.x = bz.cp[1];
	bz = o.action->curves[1].bezTriples[n];
	vec.y = bz.cp[1];
	bz = o.action->curves[2].bezTriples[n];
	vec.z = bz.cp[1];
}

void fillRotVector(WorldObject &o, UINT n, XMFLOAT3& vec) {
	BezTriple bz = o.action->curves[4].bezTriples[n]; // RotY 
	vec.x = -1.0f * bz.cp[1];
	bz = o.action->curves[3].bezTriples[n];
	vec.y = bz.cp[1];
	bz = o.action->curves[5].bezTriples[n];
	vec.z = bz.cp[1];
}

int findSegment(WorldObject &o, PathDesc *pd, double now) {
	// should really be using binary search in the already sorted array...
	for ( int i = 0; i < pd->numSegments; i++) {
		if (pd->segments[i].cumulated_length >= now) return i;
	}
	//return 0;
	throw "wrong!!!!"; // should never happen
}

void initSegments(WorldObject &o, PathDesc *pd) {
	if (pd->isBoneAnimation) {
		pd->numSegments = (int)o.boneAction->curves[0].bezTriples.size() - 1;  // #segments is #keyframes  - 1
	} else {
		pd->numSegments = (int)o.action->curves[0].bezTriples.size() - 1;  // #segments is #keyframes  - 1
	}
	pd->segments = new SegmentInfo[pd->numSegments];
	float cumulated = 0.0f;
	for ( int i = 0; i < pd->numSegments; i++) {
		UINT frames = getFrameLenghOfSegment(o, i, pd);
		float time4segmet = (frames / pd->fps) / pd->speed;
		pd->segments[i].length = time4segmet;
		pd->segments[i].cumulated_start = cumulated;
		cumulated += time4segmet;
		pd->segments[i].cumulated_length = cumulated;
	}
}

void Path::updateTime(WorldObject *o, double nowf) {
	PathDesc *pd = o->pathDescBone;
	if (pd->segments == NULL) {
		initSegments(*o, pd);
	}
	//if (pd->disableDraw != true) nowf = 0;
	//nowf = 0;
	pd->isLastPos = false; // may be reset later

	double backnowf = nowf;
	// adjust time by subtracting start time of this action:
	nowf = nowf - pd->starttime;
	// find current segment:
	float total_path_length = pd->segments[pd->numSegments-1].cumulated_length;
	while (nowf < 0.0f) // TODO strange...
		nowf = fabs(nowf);
		//nowf += (total_path_length * 200);
	if (nowf > total_path_length) {
		if (pd->pathMode == Path_SimpleMode) {
			// return last pos
			//fillPosVector(o, pd->numSegments, pos);
			pd->isLastPos = true;
			return;
		} else if (pd->pathMode == Path_Loop) {
			nowf = fmod(nowf, total_path_length);
		} else if (pd->pathMode == Path_Reverse) {
			nowf = fmod(nowf, total_path_length * 2);
			if (nowf <= total_path_length) {
				pd->currentReverseRun = false;
			} else {
				pd->currentReverseRun = true;
			}
			if (fmod(nowf, total_path_length) > total_path_length)
				throw "error";
			nowf = fmod(nowf, total_path_length);
			if (nowf > total_path_length)
				throw "error";
		}
	}
	if (pd->pathMode == Path_Reverse && pd->currentReverseRun) {
		nowf = total_path_length - nowf;
	}
	if (nowf > total_path_length)
		throw "error";
	
	if (pd->segments[pd->curSegment].cumulated_start <= nowf &&
		pd->segments[pd->curSegment].cumulated_length >= nowf) {
		// nothing to do - we are still in same segment than last time
	} else {
		int seg = findSegment(*o, pd, nowf);
		pd->curSegment = seg;
		//if (!pd->disableDraw) {
		//std::ostringstream oss;
		//oss << " wo " << o <<" time " << nowf << " segment: " << seg << " reverse: " << pd->currentReverseRun << "\n";
		//Blender::Log(oss.str());
		//}
	}
	pd->now = nowf;
	if (pd->isBoneAnimation) {
		//if (!pd->disableDraw) pd->curSegment = 0;
		//else pd->curSegment = 5;
		float segStartTime = pd->segments[pd->curSegment].cumulated_start;
		float segEndTime =   pd->segments[pd->curSegment].cumulated_length;
		assert (segStartTime <= nowf && segEndTime >= nowf);
		pd->percentage = (nowf - segStartTime) / pd->segments[pd->curSegment].length;
		//pd->percentage = 0.5f;
		//if (pd->disableDraw) {
		//std::ostringstream oss;
		//oss << "   wo " << o <<" time " << nowf << " segment: " << pd->curSegment << "\n";
		//Blender::Log(oss.str());
		////Sleep(100);
		//}
	}
}

void Path::getPos(WorldObject &o, double nowf, XMFLOAT3 &pos, XMFLOAT3 &rot) {
	//Log(" time " << setprecision(12) << nowf << endl);
	PathDesc *pd = o.pathDescMove;
	if (pd->segments == NULL) {
		initSegments(o, pd);
	}
	double backnowf = nowf;
	// adjust time by subtracting start time of this action:
	nowf = nowf - pd->starttime_f;
	// find current segment:
	float total_path_length = pd->segments[pd->numSegments-1].cumulated_length;
	while (nowf < 0.0f)
		nowf = fabs(nowf);
		//nowf += (total_path_length * 200);
	if (nowf > total_path_length) {
		if (pd->pathMode == Path_SimpleMode) {
			// return last pos
			fillPosVector(o, pd->numSegments, pos);
			pd->isLastPos = true;
			rot = o.rot();
			pos = o.pos();
			return;
		} else if (pd->pathMode == Path_Loop) {
			nowf = fmod(nowf, total_path_length);
		} else if (pd->pathMode == Path_Reverse) {
			nowf = fmod(nowf, total_path_length * 2);
			if (nowf <= total_path_length) {
				pd->currentReverseRun = false;
			} else {
				pd->currentReverseRun = true;
			}
			if (fmod(nowf, total_path_length) > total_path_length)
				throw "error";
			nowf = fmod(nowf, total_path_length);
			if (nowf > total_path_length)
				throw "error";
		}
	}
	if (pd->pathMode == Path_Reverse && pd->currentReverseRun) {
		nowf = total_path_length - nowf;
	}
	if (nowf > total_path_length)
		throw "error";
	
	if (pd->segments[pd->curSegment].cumulated_start <= nowf &&
		pd->segments[pd->curSegment].cumulated_length >= nowf) {
		// nothing to do - we are still in same segment than last time
	} else {
		int seg = findSegment(o, pd, nowf);
		pd->curSegment = seg;
		//std::ostringstream oss;
		//oss << " time " << nowf << " segment: " << seg << " reverse: " << pd->currentReverseRun << "\n";
		//Blender::Log(oss.str());
	}

	if (o.action->curves[0].bezTriples[0].isBoneAnimation) {
		//std::ostringstream oss;
		//oss << " time " << nowf << " segment: " << pd->curSegment << " reverse: " << pd->currentReverseRun << "\n";
		//Blender::Log(oss.str());
		// set pos, rot to zero (no global movement)
		pos.x = pos.y = pos.z = 0;
		rot.x = rot.y = rot.z = 0;
		return;
	}
	// handle translation:
	XMFLOAT3 p0;
	fillPosVector(o, pd->curSegment, p0);
	XMFLOAT3 p1;
	fillPosVector(o, pd->curSegment + 1, p1);
	XMVECTOR segment = XMLoadFloat3(&p1) - XMLoadFloat3(&p0);
	float len = XMVectorGetX(XMVector3Length(segment));
	if (len == 0.0f) {
		pos = p0;	// no movement
	} else {
		double passed_segment = nowf - pd->segments[pd->curSegment].cumulated_start;
		double length_segment = pd->segments[pd->curSegment].length;
		XMVECTOR scale = XMVectorScale(segment, (float)(passed_segment / length_segment));
		scale = XMLoadFloat3(&p0) + scale;
		float len = XMVectorGetX(XMVector3Length(scale));
		//Log("scale " << len << endl);
		XMStoreFloat3(&pos, scale);
	}

	// handle rotation:
	if (pd->handleRotation == false) {
		// just copy back original rotation - no chnges along path movement
		rot = o.rot();
	} else {
		rot = XMFLOAT3(0.0f, 0.0f, 0.0f);
		XMFLOAT3 r0;
		fillRotVector(o, pd->curSegment, r0);
		XMFLOAT3 r1;
		fillRotVector(o, pd->curSegment + 1, r1);
		XMVECTOR segmentRot = XMLoadFloat3(&r1) - XMLoadFloat3(&r0);
		float lenRot = XMVectorGetX(XMVector3Length(segmentRot));
		if (lenRot == 0.0f) {
			rot = r0;	// no rotation
		}
		else {
			double passed_segment = nowf - pd->segments[pd->curSegment].cumulated_start;
			double length_segment = pd->segments[pd->curSegment].length;
			XMVECTOR scale = XMVectorScale(segmentRot, (float)(passed_segment / length_segment));
			scale = XMLoadFloat3(&r0) + scale;
			XMStoreFloat3(&rot, scale);
		}
	}

	// return 1st pos
	//D3DXVECTOR3 p0;
	//fillPosVector(o, pd->curSegment, p0);
	//pos = p0;
}

/*void getPos(WorldObject &o, float nowf, LONGLONG now, LONGLONG ticks_per_second, D3DXVECTOR3 &pos) {
	PathDesc *pd = o.pathDesc;
	if (pd->curSegment == -1 || pd->curSegment >= pd->numSegments) {
		fillPosVector(o, pd->numSegments, pos);
		pd->segment_start_pos = pos;
		//reset
		pd->starttime = 0L;
		pd->starttime_f = 0.0f;
		pd->curSegment = 0;
	    return;
	}
	
	if (pd->starttime == 0L) {
        pd->starttime = now;
		pd->starttime_f = nowf;
		fillPosVector(o, pd->curSegment, pos);
		pd->segment_start_pos = pos;
		return;
	}
	D3DXVECTOR3 p0;
	fillPosVector(o, pd->curSegment, p0);
	//D3DXVECTOR3 &p0 = pd->pos[pd->curSegment];
	D3DXVECTOR3 p1;
	fillPosVector(o, pd->curSegment + 1, p1);
	//D3DXVECTOR3 &p1 = pd->pos[pd->curSegment + 1];
	D3DXVECTOR3 segment = p1 - p0;
	float len = D3DXVec3Length(&segment);
	float num_ticks = len / pd->speed * 100000;
	LONGLONG passed = now - pd->starttime;
	float passedf = nowf - pd->starttime_f;
	float time4segmet = len / pd->speed;
	float len_cur;
	UINT frames = getFrameLenghOfSegment(o, pd->curSegment);
	time4segmet = frames / pd->fps;
	if (len == 0.0f) {
		// pausing object
		len_cur = 0.0f;
	} else {
		len_cur = len * passedf / time4segmet;
	}

	//if (passed > num_ticks) {
	if (passedf > time4segmet) {
		// advanced past next segment start
		pd->curSegment++;
		if (pd->curSegment >= pd->numSegments)
			pd->curSegment = -1;
		pd->starttime = now-100;
		pd->starttime_f = nowf;
		return getPos(o, nowf, now, (LONGLONG)num_ticks, pos);
	}

*/	/*
	WCHAR infoLine[256];
	WCHAR* pInfoLine = infoLine;
	StringCchPrintf(pInfoLine, 256, L"p0 = (%.2f,%.2f,%.2f) p1 = (%.2f,%.2f,%.2f) scale %.2f %I64d\n",
		p0.x, p0.y, p0.z, p1.x, p1.y, p1.z, passed/num_ticks, now);
	OutputDebugString(pInfoLine);
	*/
/*	if (passed / num_ticks < 0) {
		throw "wrong!!!!"; // should never happen
	}
*/	//pd->currentPos = *D3DXVec3Scale( &pd->currentPos, &segment, passed / num_ticks);
	//pos = pd->currentPos;
	// new time concept:
/*	if (len_cur > 0.0f)
		pd->currentPos = *D3DXVec3Scale( &pd->currentPos, &segment, len_cur);
	pos = pd->currentPos;
	return;
}*/

/*
XMVECTOR Path::skin(const XMVECTOR v_in, const XMMATRIX &ibm, const XMMATRIX &jm, float jw, float scale) 
{
	XMMATRIX IBM = XMMatrixTranspose(ibm);
	XMMATRIX JM = XMMatrixTranspose(jm);
	XMVECTOR v = v_in * scale;
	v = XMVector3Transform(v, IBM);
	v = XMVector3Transform(v, JM);
	v = v * jw;
	return v;
}

XMVECTOR Path::skin(const XMVECTOR v, const XMMATRIX &ibm, int boneIndex, const std::vector<Curve> *curves, int segment, float jw, float scale)
{
	int boneId = 0;
	static XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMMATRIX vm = XMMATRIX((*curves)[boneId].bezTriples[segment].transMatrix);
	//XMMATRIX vm = (*curves)[boneId].bezTriples[segment].transMatrix;
	XMVECTOR vm_scale, vm_rotq, vm_trans;
	bool success = XMMatrixDecompose(&vm_scale, &vm_rotq, &vm_trans, vm);
	if (success) {
		//vm_trans = XMVectorSetZ(vm_trans, XMVectorGetZ(vm_trans) - 10.0f);
		XMMATRIX vm2 = XMMatrixAffineTransformation(vm_scale, zero, vm_rotq, vm_trans);
		vm = vm2;
	}
	while (boneId < boneIndex) {
		boneId++; 
		//vm = vm * (*curves)[boneId].bezTriples[segment].transMatrix;
		vm = vm * XMMATRIX((*curves)[boneId].bezTriples[segment].transMatrix);
	}
	return skin(v, ibm, vm, jw, scale);
}
*/

// assuming we have a transformation matrix without scale we can just use uppel left 3x3 as rotation matrix
XMVECTOR rotate(XMMATRIX m, XMVECTOR v) {
	XMVECTOR rotation_quaternion = XMQuaternionRotationMatrix(m);
	rotation_quaternion = XMQuaternionNormalize(rotation_quaternion);
	// rotate v
	return XMVector3Rotate(v, rotation_quaternion);
}

void  Path::skinNonKeyframe(XMVECTOR &pos, XMVECTOR &norm, const WorldObjectVertex::VertexSkinned *v, PathDesc *pd)
{
	XMVECTOR vskin = XMVectorZero();
	XMVECTOR normskin = XMVectorZero();
	for (int i = 0; i < 4; i++) {
		BYTE boneIndex = v->BoneIndices[i];
		float weight;
		switch (i) {
		case 0: weight = v->Weights.x; break;
		case 1: weight = v->Weights.y; break;
		case 2: weight = v->Weights.z; break;
		case 3: weight = v->Weights.w; break;
		}
		if (boneIndex == 0xff || weight == 0.0f) continue;
		XMMATRIX ibm = XMLoadFloat4x4(&pd->clip->invBindMatrices[boneIndex]);
		ibm = XMMatrixTranspose(ibm);
		XMVECTOR r = XMLoadFloat3(&v->Pos);
		XMVECTOR n = XMLoadFloat3(&v->Normal);
		//r = XMVector3Transform(r, ibm);
		//n = rotate(ibm, n);
		//XMMATRIX m(XMLoadFloat4x4(&pd->interpolationMatricesChained[boneIndex]));
		XMMATRIX imc = getInterpolationMatrixChained(boneIndex, pd);
		r = XMVector3Transform(r, imc);
		n = rotate(imc, n);
		r = r * weight;
		n = n * weight;
		vskin += r;
		normskin += n;
	}
	pos = vskin;
	norm = XMVector3Normalize(normskin); //XMLoadFloat3(&v->Normal);
										 //float len = XMVectorGetX(XMVector3Length(norm));
										 //Log("normal length = " << len << endl);
}

void  Path::skin(XMVECTOR &pos, XMVECTOR &norm, const WorldObjectVertex::VertexSkinned *v, PathDesc *pd)
{
	XMVECTOR vskin = XMVectorZero();
	XMVECTOR normskin = XMVectorZero();
	for (int i = 0; i < 4; i++) {
		BYTE boneIndex = v->BoneIndices[i];
		float weight;
		switch (i) {
		case 0: weight = v->Weights.x; break;
		case 1: weight = v->Weights.y; break;
		case 2: weight = v->Weights.z; break;
		case 3: weight = v->Weights.w; break;
		}
		if (boneIndex == 0xff || weight == 0.0f) continue;
		XMMATRIX ibm = XMLoadFloat4x4(&pd->clip->invBindMatrices[boneIndex]);
		ibm = XMMatrixTranspose(ibm);
		XMVECTOR r = XMLoadFloat3(&v->Pos);
		XMVECTOR n = XMLoadFloat3(&v->Normal);
		r = XMVector3Transform(r, ibm);
		n = rotate(ibm, n);
		//XMMATRIX m(XMLoadFloat4x4(&pd->interpolationMatricesChained[boneIndex]));
		XMMATRIX imc = getInterpolationMatrixChained(boneIndex, pd);
		r = XMVector3Transform(r, imc);
		n = rotate(imc, n);
		r = r * weight;
		n = n * weight;
		vskin += r;
		normskin += n;
	}
	pos = vskin;
	norm = XMVector3Normalize(normskin); //XMLoadFloat3(&v->Normal);
	//float len = XMVectorGetX(XMVector3Length(norm));
	//Log("normal length = " << len << endl);
}

/*
XMVECTOR Path::skin(const Vertex::Skinned *v, const AnimationClip *clip, const std::vector<Curve> *curves, int segment, float scale, float percentage)
{
	static XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	static const std::vector<Curve> *lastCurves = 0;
	static float lastPercentage = -1.0f;
	static int lastSegment = -1;
	int numBones = clip->invBindMatrices.size();
	int numKeyframes = curves->at(0).bezTriples.size();
	//assert (numBones == 2 && numKeyframes == 3);
	//assert (numBones == curves->size());

	if (lastCurves != curves || lastPercentage != percentage || lastSegment != segment) {
		lastCurves = curves;
		lastPercentage = percentage;
		lastSegment = segment;
		// recalculate interpolation matrices:
		for (int i = numBones - 1; i >= 0; i--) {
			// decompose segment start
			XMMATRIX transform0 = XMMATRIX((*curves)[i].bezTriples[segment].transMatrix);
			transform0 = XMMatrixTranspose(transform0);
			XMVECTOR s0, r0, t0;
			bool success = XMMatrixDecompose(&s0, &r0, &t0, transform0);
			assert(success);
			// decompose segment end
			XMMATRIX transform1 = XMMATRIX((*curves)[i].bezTriples[segment + 1].transMatrix);
			transform1 = XMMatrixTranspose(transform1);
			XMVECTOR s1, r1, t1;
			success = XMMatrixDecompose(&s1, &r1, &t1, transform1);
			assert(success);
			// interpolate
			XMVECTOR s, r, t;
			s = XMVectorLerp(s0, s1, percentage);
			r = XMQuaternionSlerp(r0, r1, percentage);
			t = XMVectorLerp(t0, t1, percentage);
			// reassemble transformation matrix
			///////////interpolationMatrices[i] = XMMatrixAffineTransformation(s, zero, r, t);
			//interpolationMatrices[i] = XMMatrixAffineTransformation(s1, zero, r1, t1);
		}
		////calculateInterpolationChain(clip);
	}
	XMVECTOR vskin = XMVectorZero();
	for (int i = 0; i < 4; i++) {
		BYTE boneIndex = v->BoneIndices[i];
		float weight;
		switch (i) {
		case 0: weight = v->Weights.x; break;
		case 1: weight = v->Weights.y; break;
		case 2: weight = v->Weights.z; break;
		case 3: weight = v->Weights.w; break;
		}
		if (boneIndex == 0xff || weight == 0.0f) continue;
		XMMATRIX ibm = XMLoadFloat4x4(&clip->invBindMatrices[boneIndex]);
		ibm = XMMatrixTranspose(ibm);
		XMVECTOR r = XMLoadFloat3(&v->Pos) * scale;
		r = XMVector3Transform(r, ibm);
		//////////////r = XMVector3Transform(r, interpolationMatricesChained[boneIndex]);
		r = r * weight;
		vskin += r;
	}
	return vskin;
}
*/
void Path::updateBindPose(int i, PathDesc * pd, XMMATRIX * m)
{
	saveInterpolationMatrix(i, pd, m);
}

XMMATRIX Path::getInterpolationMatrix(int i, PathDesc *pd) {
	return XMLoadFloat4x4(&pd->interpolationMatrices[i]);
}

void Path::saveInterpolationMatrix(int i, PathDesc *pd, XMMATRIX *m) {
	XMStoreFloat4x4(&pd->interpolationMatrices[i], *m);
}

XMMATRIX Path::getInterpolationMatrixChained(int i, PathDesc *pd) {
	return XMLoadFloat4x4(&pd->interpolationMatricesChained[i]);
}

void Path::saveInterpolationMatrixChained(int i, PathDesc *pd, XMMATRIX *m) {
	XMStoreFloat4x4(&pd->interpolationMatricesChained[i], *m);
}


//void copyMatrix(XMMATRIX to, XMMATRIX from) {
//}

void Path::calculateInterpolationChain(const AnimationClip *clip, PathDesc *pd)
{
	//for (int i = 0; i < 2; i++) {
	//	interpolationMatricesChained[i] = interpolationMatrices[i];
	//	if (i == 1) interpolationMatricesChained[i] = interpolationMatricesChained[1] * interpolationMatricesChained[0];
	//}
	for (int i = 0; i < clip->numBones; i++) {
		// bones are guaranteed to be in parent first order
		int parentofThisBone = clip->parents[i];
		if ( parentofThisBone < 0) {
			// root
			saveInterpolationMatrixChained(i, pd, &getInterpolationMatrix(i, pd));
			//pd->interpolationMatricesChained[i] = pd->interpolationMatrices[i];
		} else {
			XMMATRIX m = getInterpolationMatrix(i, pd) * getInterpolationMatrixChained(parentofThisBone, pd);
			saveInterpolationMatrixChained(i, pd, &m);
			//pd->interpolationMatricesChained[i] = pd->interpolationMatrices[i] * pd->interpolationMatricesChained[parentofThisBone];
		}
	}
}

void::Path::updateScene(PathDesc *pathDesc, WorldObject *wo, double time)
{
	if (wo->isNonKeyframeAnimated) {
		recalculateBoneAnimation(wo);
		return;
	}
	// currently only animation clips are updated here, path moving should be too...
	if (wo->boneAction == 0) {
		return;
	}
	updateTime(wo, time);
	if (wo->boneAction) {
		recalculateBoneAnimation(pathDesc, wo, pathDesc->percentage);
		//Log(" percentage " << pathDesc->percentage << " pathDesc " << pathDesc << "\n");
	}
}

// non-keyframe animation
void Path::recalculateBoneAnimation(WorldObject *wo)
{
	const AnimationClip *clip = wo->pathDescBone->clip;
	PathDesc *pathDesc = wo->pathDescBone;
	int numBones = (int)clip->invBindMatrices.size();
	for (int i = numBones - 1; i >= 0; i--) {
		const XMFLOAT4X4 *xmm4 = &clip->invBindMatrices[i];
		XMMATRIX xmm = XMLoadFloat4x4(xmm4);
		xmm = getInterpolationMatrix(i, pathDesc) * xmm;
		saveInterpolationMatrix(i, pathDesc, &xmm);
	}
	calculateInterpolationChain(clip, pathDesc);
}

void Path::recalculateBoneAnimation(PathDesc *pathDesc, WorldObject *wo, double percentage) 
{
	XMVECTOR zero = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	int numBones = (int)pathDesc->clip->invBindMatrices.size();
	//int numKeyframes = curves->at(0).bezTriples.size();
	//assert (numBones == 2 && numKeyframes == 3);
	//assert (numBones == curves->size());

	//if (pathDesc->lastCurves != &wo->boneAction->curves || pathDesc->lastPercentage != percentage || pathDesc->lastSegment != pathDesc->curSegment) {
	if (true) {
		const std::vector<Curve> *curves = &wo->boneAction->curves;
		int segment = pathDesc->curSegment;
		pathDesc->lastCurves = curves;
		pathDesc->lastPercentage = percentage;
		pathDesc->lastSegment = segment;
		// recalculate interpolation matrices:
		for (int i = numBones-1; i >= 0; i-- ) {
			// decompose segment start
			XMMATRIX transform0 = XMMATRIX((*curves)[i].bezTriples[segment].transMatrix);
			transform0 = XMMatrixTranspose(transform0);
			XMVECTOR s0, r0, t0;
			bool success = XMMatrixDecompose(&s0, &r0, &t0, transform0);
			assert(success);
			// decompose segment end
			XMMATRIX transform1 = XMMATRIX((*curves)[i].bezTriples[segment+1].transMatrix);
			transform1 = XMMatrixTranspose(transform1);
			XMVECTOR s1, r1, t1;
			success = XMMatrixDecompose(&s1, &r1, &t1, transform1);
			assert(success);
			// interpolate
			XMVECTOR s, r, t;
			s = XMVectorLerp(s0, s1, (float)percentage);
			r = XMQuaternionSlerp(r0, r1, (float)percentage);
			t = XMVectorLerp(t0, t1, (float)percentage);
			// reassemble transformation matrix
			XMMATRIX xmm = XMMatrixAffineTransformation(s, zero, r, t);
			//pathDesc->interpolationMatrices[i]._11 = xmm._11;
			//pathDesc->interpolationMatrices[i]._12 = xmm._12;
			//pathDesc->interpolationMatrices[i]._13 = xmm._13;
			//pathDesc->interpolationMatrices[i]._14 = xmm._14;
			//pathDesc->interpolationMatrices[i]._21 = xmm._21;
			//pathDesc->interpolationMatrices[i]._22 = xmm._22;
			//pathDesc->interpolationMatrices[i]._23 = xmm._23;
			//pathDesc->interpolationMatrices[i]._24 = xmm._24;
			//pathDesc->interpolationMatrices[i]._31 = xmm._31;
			//pathDesc->interpolationMatrices[i]._32 = xmm._32;
			//pathDesc->interpolationMatrices[i]._33 = xmm._33;
			//pathDesc->interpolationMatrices[i]._34 = xmm._34;
			//pathDesc->interpolationMatrices[i]._41 = xmm._41;
			//pathDesc->interpolationMatrices[i]._42 = xmm._42;
			//pathDesc->interpolationMatrices[i]._43 = xmm._43;
			//pathDesc->interpolationMatrices[i]._44 = xmm._44;
			saveInterpolationMatrix(i, pathDesc, &xmm);
			//pathDesc->interpolationMatrices[i] = xmm;
			//interpolationMatrices[i] = XMMatrixAffineTransformation(s1, zero, r1, t1);
		}
		calculateInterpolationChain(pathDesc->clip, pathDesc);
	}

}

/*void Path::playClip(WorldObject *wo, char *clipName)
{
}*/


Path::Path(void)
{
}

Path::~Path(void)
{
}

// on-the-fly path composition:
/*
action = new Action();
action->name = std::string(ani_name);
for (int i = 0; i < 9; i++) {
Curve curve;
int numSegments;
bfile.read((char*)&numSegments, 4);
for (int j = 0; j < numSegments; j++) {
BezTriple b;
b.isBoneAnimation = false;
bfile.read((char*)&b.h1[0], 4);
bfile.read((char*)&b.h1[1], 4);
bfile.read((char*)&b.cp[0], 4);
bfile.read((char*)&b.cp[1], 4);
bfile.read((char*)&b.h2[0], 4);
bfile.read((char*)&b.h2[1], 4);
curve.bezTriples.push_back(b);
}
action->curves.push_back(curve);
}
mesh->actions[action->name] = *action;
delete action;

void fillPosVector(WorldObject &o, UINT n, XMFLOAT3& vec) {
BezTriple bz = o.action->curves[0].bezTriples[n];
vec.x = bz.cp[1];
bz = o.action->curves[1].bezTriples[n];
vec.y = bz.cp[1];
bz = o.action->curves[2].bezTriples[n];
vec.z = bz.cp[1];
}

void fillRotVector(WorldObject &o, UINT n, XMFLOAT3& vec) {
BezTriple bz = o.action->curves[4].bezTriples[n]; // RotY
vec.x = -1.0f * bz.cp[1];
bz = o.action->curves[3].bezTriples[n];
vec.y = bz.cp[1];
bz = o.action->curves[5].bezTriples[n];
vec.z = bz.cp[1];
}
*/

// totalTime in seconds
void Path::adjustTimings(vector<XMFLOAT4> &p, float totalTime) {
	float totalLen = 0.0f;
	for (int i = 1; i < p.size(); i++) {
		// store length of segment 
		XMFLOAT3 p1p = XMFLOAT3(p[i - 1].x, p[i - 1].y, p[i - 1].z);
		XMFLOAT3 p2p = XMFLOAT3(p[i].x, p[i].y, p[i].z);
		XMVECTOR p1 = XMLoadFloat3(&p1p);
		XMVECTOR p2 = XMLoadFloat3(&p2p);
		float len = XMVectorGetX(XMVector3Length(p2 - p1));
		totalLen += len;
		p[i].w = len;  // temporary store len
	}
	float speedfactor = totalTime / totalLen;
	float totalFPSTime = p[0].w;
	for (int i = 1; i < p.size(); i++) {
		float segmentTime = p[i].w * speedfactor;
		p[i].w = (segmentTime * 25.0f) + totalFPSTime; // FPS
		totalFPSTime += segmentTime * 25.0f;
	}
}

void Path::defineAction(char* name, WorldObject & wo, vector<XMFLOAT4>& ctrlPoints, vector<XMFLOAT3> *rot)
{
	if (rot != nullptr) {
		assert(rot->size() == ctrlPoints.size());
	}
	if (wo.mesh->actions.count(name)) {
		// key already exists, do a cleanup
		if (wo.pathDescMove->segments) delete[] wo.pathDescMove->segments;
		delete wo.pathDescMove;
	}
	auto action = new Action();
	action->name = std::string(name);
	for (int i = 0; i < 9; i++) {
		Curve curve;
		int numSegments = ctrlPoints.size();
		//bfile.read((char*)&numSegments, 4);
		for (int j = 0; j < numSegments; j++) {
			BezTriple b;
			b.isBoneAnimation = false;
			b.h1[0] = 0.0f;
			b.h1[1] = 0.0f;
			b.h2[0] = 0.0f;
			b.h2[1] = 0.0f;
			switch (i) {
			case 0:
				b.cp[0] = ctrlPoints[j].w; // frame/time index
				b.cp[1] = ctrlPoints[j].x;
				break;
			case 1:
				b.cp[0] = ctrlPoints[j].w; // frame/time index
				b.cp[1] = ctrlPoints[j].y;
				break;
			case 2:
				b.cp[0] = ctrlPoints[j].w; // frame/time index
				b.cp[1] = ctrlPoints[j].z;
				break;
			}
			if (rot != nullptr) {
				switch (i) {
				case 4:
					b.cp[0] = ctrlPoints[j].w; // frame/time index
					b.cp[1] = (*rot)[j].y;
					break;
				case 3:
					b.cp[0] = ctrlPoints[j].w; // frame/time index
					b.cp[1] = -1.0f * (*rot)[j].x;
					break;
				case 5:
					b.cp[0] = ctrlPoints[j].w; // frame/time index
					b.cp[1] = (*rot)[j].z;
					break;
				}
			}
			curve.bezTriples.push_back(b);
		}
		action->curves.push_back(curve);
	}
	wo.mesh->actions[action->name] = *action;
	delete action;
}

