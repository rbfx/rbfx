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

#pragma once


#include <Urho3D/Core/Object.h>

namespace Urho3D
{

class EditorIconCache : public Object
{
    URHO3D_OBJECT(EditorIconCache, Object)
public:
    struct IconData
    {
        /// A texture which contains the icon.
        ResourceRef textureRef_;
        /// Icon location and size.
        IntRect rect_;
    };

    /// Reads EditorIcons.xml and stores information for later use by imgui.
    explicit EditorIconCache(Context* context);
    /// Return pointer to icon data.
    IconData* GetIconData(const String& name);

protected:
    /// Editor icon cache.
    HashMap<String, IconData> iconCache_;
};

}
