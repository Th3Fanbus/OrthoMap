/* SPDX-License-Identifier: MPL-2.0 */

#include "OrthoMapBPFL.h"

FVector2D UOrthoMapBPFL::QuantizeVector2D(FVector2D InVec, double QuantStep)
{
	return (InVec / QuantStep).RoundToVector() * QuantStep;
}
