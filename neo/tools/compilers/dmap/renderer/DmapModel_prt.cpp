/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#pragma hdrstop
#include "precompiled.h"

#include "Dmap_tr_local.h"
#include "DmapModel_local.h"


static const char *parametricParticle_SnapshotName = "_ParametricParticle_Snapshot_";

/*
====================
idDmapRenderModelPrt::idDmapRenderModelPrt
====================
*/
idDmapRenderModelPrt::idDmapRenderModelPrt() {
	particleSystem = NULL;
}

/*
====================
idDmapRenderModelPrt::InitFromFile
====================
*/
void idDmapRenderModelPrt::InitFromFile(const char *fileName) {
	name = fileName;
	particleSystem = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, fileName));
}

/*
=================
idDmapRenderModelPrt::TouchData
=================
*/
void idDmapRenderModelPrt::TouchData(void) {
	// Ensure our particle system is added to the list of referenced decls
	particleSystem = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, name));
}

/*
====================
idDmapRenderModelPrt::InstantiateDynamicModel
====================
*/
idDmapRenderModel *idDmapRenderModelPrt::InstantiateDynamicModel(const struct dmapRenderEntity_s *renderEntity, const struct dmapViewDef_s *viewDef, idDmapRenderModel *cachedModel) {
	idDmapRenderModelStatic	*staticModel = NULL;

	assert(0);
	/*
	if (cachedModel && !r_useCachedDynamicModels.GetBool()) {
		delete cachedModel;
		cachedModel = NULL;
	}

	// this may be triggered by a model trace or other non-view related source, to which we should look like an empty model
	if (renderEntity == NULL || viewDef == NULL) {
		delete cachedModel;
		return NULL;
	}

	if (r_skipParticles.GetBool()) {
		delete cachedModel;
		return NULL;
	}

	if (cachedModel != NULL) {

		assert(dynamic_cast<idDmapRenderModelStatic *>(cachedModel) != NULL);
		assert(idStr::Icmp(cachedModel->Name(), parametricParticle_SnapshotName) == 0);

		staticModel = static_cast<idDmapRenderModelStatic *>(cachedModel);

	}
	else {

		staticModel = new idDmapRenderModelStatic;
		staticModel->InitEmpty(parametricParticle_SnapshotName);
	}

	particleGen_t g;

	g.renderEnt = renderEntity;
	g.renderView = &viewDef->renderView;
	g.origin.Zero();
	g.axis.Identity();

	for (int stageNum = 0; stageNum < particleSystem->stages.Num(); stageNum++) {
		idParticleStage *stage = particleSystem->stages[stageNum];

		if (!stage->material) {
			continue;
		}
		if (!stage->cycleMsec) {
			continue;
		}
		if (stage->hidden) {		// just for gui particle editor use
			staticModel->DeleteSurfaceWithId(stageNum);
			continue;
		}

		idRandom steppingRandom, steppingRandom2;

		int stageAge = g.renderView->time + renderEntity->shaderParms[SHADERPARM_TIMEOFFSET] * 1000 - stage->timeOffset * 1000;
		int	stageCycle = stageAge / stage->cycleMsec;

		// some particles will be in this cycle, some will be in the previous cycle
		steppingRandom.SetSeed(((stageCycle << 10) & idRandom::MAX_RAND) ^ (int)(renderEntity->shaderParms[SHADERPARM_DIVERSITY] * idRandom::MAX_RAND));
		steppingRandom2.SetSeed((((stageCycle - 1) << 10) & idRandom::MAX_RAND) ^ (int)(renderEntity->shaderParms[SHADERPARM_DIVERSITY] * idRandom::MAX_RAND));

		int	count = stage->totalParticles * stage->NumQuadsPerParticle();

		int surfaceNum;
		modelSurface_t *surf;

		if (staticModel->FindSurfaceWithId(stageNum, surfaceNum)) {
			surf = &staticModel->surfaces[surfaceNum];
			R_FreeStaticTriSurfVertexCaches(surf->geometry);
		}
		else {
			surf = &staticModel->surfaces.Alloc();
			surf->id = stageNum;
			surf->shader = stage->material;
			surf->geometry = R_AllocStaticTriSurf();
			R_AllocStaticTriSurfVerts(surf->geometry, 4 * count);
			R_AllocStaticTriSurfIndexes(surf->geometry, 6 * count);
			R_AllocStaticTriSurfPlanes(surf->geometry, 6 * count);
		}

		int numVerts = 0;
		idDrawVert *verts = surf->geometry->verts;

		for (int index = 0; index < stage->totalParticles; index++) {
			g.index = index;

			// bump the random
			steppingRandom.RandomInt();
			steppingRandom2.RandomInt();

			// calculate local age for this index
			int	bunchOffset = stage->particleLife * 1000 * stage->spawnBunching * index / stage->totalParticles;

			int particleAge = stageAge - bunchOffset;
			int	particleCycle = particleAge / stage->cycleMsec;
			if (particleCycle < 0) {
				// before the particleSystem spawned
				continue;
			}
			if (stage->cycles && particleCycle >= stage->cycles) {
				// cycled systems will only run cycle times
				continue;
			}

			if (particleCycle == stageCycle) {
				g.random = steppingRandom;
			}
			else {
				g.random = steppingRandom2;
			}

			int	inCycleTime = particleAge - particleCycle * stage->cycleMsec;

			if (renderEntity->shaderParms[SHADERPARM_PARTICLE_STOPTIME] &&
				g.renderView->time - inCycleTime >= renderEntity->shaderParms[SHADERPARM_PARTICLE_STOPTIME] * 1000) {
				// don't fire any more particles
				continue;
			}

			// supress particles before or after the age clamp
			g.frac = (float)inCycleTime / (stage->particleLife * 1000);
			if (g.frac < 0.0f) {
				// yet to be spawned
				continue;
			}
			if (g.frac > 1.0f) {
				// this particle is in the deadTime band
				continue;
			}

			// this is needed so aimed particles can calculate origins at different times
			g.originalRandom = g.random;

			g.age = g.frac * stage->particleLife;

			// if the particle doesn't get drawn because it is faded out or beyond a kill region, don't increment the verts
			numVerts += stage->CreateParticle(&g, verts + numVerts);
		}

		// numVerts must be a multiple of 4
		assert((numVerts & 3) == 0 && numVerts <= 4 * count);

		// build the indexes
		int	numIndexes = 0;
		glIndex_t *indexes = surf->geometry->indexes;
		for (int i = 0; i < numVerts; i += 4) {
			indexes[numIndexes + 0] = i;
			indexes[numIndexes + 1] = i + 2;
			indexes[numIndexes + 2] = i + 3;
			indexes[numIndexes + 3] = i;
			indexes[numIndexes + 4] = i + 3;
			indexes[numIndexes + 5] = i + 1;
			numIndexes += 6;
		}

		surf->geometry->tangentsCalculated = false;
		surf->geometry->facePlanesCalculated = false;
		surf->geometry->numVerts = numVerts;
		surf->geometry->numIndexes = numIndexes;
		surf->geometry->bounds = stage->bounds;		// just always draw the particles
	}*/

	return staticModel;
}

/*
====================
idDmapRenderModelPrt::IsDynamicModel
====================
*/
dynamicModel_t idDmapRenderModelPrt::IsDynamicModel() const {
	return DM_CONTINUOUS;
}

/*
====================
idDmapRenderModelPrt::Bounds
====================
*/
idBounds idDmapRenderModelPrt::Bounds(const struct dmapRenderEntity_s *ent) const {
	return particleSystem->bounds;
}

/*
====================
idDmapRenderModelPrt::DepthHack
====================
*/
float idDmapRenderModelPrt::DepthHack() const {
	return particleSystem->depthHack;
}

/*
====================
idDmapRenderModelPrt::Memory
====================
*/
int idDmapRenderModelPrt::Memory() const {
	int total = 0;

	total += idDmapRenderModelStatic::Memory();

	if (particleSystem) {
		total += sizeof(*particleSystem);

		for (int i = 0; i < particleSystem->stages.Num(); i++) {
			total += sizeof(particleSystem->stages[i]);
		}
	}

	return total;
}
