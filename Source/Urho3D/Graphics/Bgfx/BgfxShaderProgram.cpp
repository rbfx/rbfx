//
// Copyright (c) 2008-2017 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../../Precompiled.h"

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/ShaderProgram.h"
#include "../../Graphics/ShaderVariation.h"
#include "../../IO/Log.h"

namespace Urho3D
{

ShaderProgram::ShaderProgram(Graphics* graphics, ShaderVariation* vertexShader, ShaderVariation* pixelShader)
{
	for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
	{
		texSamplers_[i].idx = bgfx::kInvalidHandle;
	}

	bgfx::ShaderHandle vsh;
	bgfx::ShaderHandle fsh;

	vsh.idx = vertexShader->GetGPUObjectIdx();
	fsh.idx = vertexShader->GetGPUObjectIdx();

	handle_ = bgfx::createProgram(vsh, fsh, false);

	if (!bgfx::isValid(handle_))
		URHO3D_LOGERROR("Failed to create BGFX program");

	// Copy parameters, add direct links to constant buffers
	const HashMap<StringHash, ShaderParameter>& vsParams = vertexShader->GetParameters();
	for (HashMap<StringHash, ShaderParameter>::ConstIterator i = vsParams.Begin(); i != vsParams.End(); ++i)
	{
		parameters_[i->first_] = i->second_;
		// Map texture unit number to uniform handle
		if ((i->second_.type_ == bgfx::UniformType::Int1) && (i->second_.texUnit_ < MAX_TEXTURE_UNITS))
			texSamplers_[i->second_.texUnit_].idx = i->second_.idx_;
	}

	const HashMap<StringHash, ShaderParameter>& psParams = pixelShader->GetParameters();
	for (HashMap<StringHash, ShaderParameter>::ConstIterator i = psParams.Begin(); i != psParams.End(); ++i)
	{
		parameters_[i->first_] = i->second_;
		// Map texture unit number to uniform handle
		if ((i->second_.bgfxType_ == bgfx::UniformType::Int1) && (i->second_.texUnit_ < MAX_TEXTURE_UNITS))
			texSamplers_[i->second_.texUnit_].idx = i->second_.idx_;
	}

	// Optimize shader parameter lookup by rehashing to next power of two
	parameters_.Rehash(NextPowerOfTwo(parameters_.Size()));
}

ShaderProgram::~ShaderProgram()
{
	bgfx::destroy(handle_);
}

}
