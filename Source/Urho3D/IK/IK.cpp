// Copyright (c) 2022-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/IK/IK.h"

#include "Urho3D/IK/AllSolvers.h"
#include "Urho3D/IK/IKSolver.h"

namespace Urho3D
{

void RegisterIKLibrary(Context* context)
{
    IKSolver::RegisterObject(context);
    IKSolverComponent::RegisterObject(context);

    IKIdentitySolver::RegisterObject(context);
    IKRotateTo::RegisterObject(context);
    IKLegSolver::RegisterObject(context);
    IKLimbSolver::RegisterObject(context);
    IKSpineSolver::RegisterObject(context);
    IKArmSolver::RegisterObject(context);
    IKChainSolver::RegisterObject(context);
    IKHeadSolver::RegisterObject(context);

    IKStickTargets::RegisterObject(context);
}

} // namespace Urho3D
