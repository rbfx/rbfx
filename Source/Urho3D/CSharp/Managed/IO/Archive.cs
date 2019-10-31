namespace Urho3DNet
{
    public partial class Archive
    {
        /// Do BeginBlock and return the guard that will call EndBlock automatically on destruction.
        /// Return null block in case of error.
        ArchiveBlock OpenBlock(string name, uint sizeHint, bool safe, ArchiveBlockType type)
        {
            bool opened = BeginBlock(name, ref sizeHint, safe, type);
            return opened ? new ArchiveBlock(this, sizeHint) : new ArchiveBlock();
        }

        /// Open Sequential block. Will be automatically closed when returned object is destroyed.
        ArchiveBlock OpenSequentialBlock(string name) { return OpenBlock(name, 0, false, ArchiveBlockType.Sequential); }
        /// Open Unordered block. Will be automatically closed when returned object is destroyed.
        ArchiveBlock OpenUnorderedBlock(string name) { return OpenBlock(name, 0, false, ArchiveBlockType.Unordered); }
        /// Open Array block. Will be automatically closed when returned object is destroyed.
        ArchiveBlock OpenArrayBlock(string name, uint sizeHint = 0) { return OpenBlock(name, sizeHint, false, ArchiveBlockType.Array); }
        /// Open Map block. Will be automatically closed when returned object is destroyed.
        ArchiveBlock OpenMapBlock(string name, uint sizeHint = 0) { return OpenBlock(name, sizeHint, false, ArchiveBlockType.Map); }

        /// Open safe Sequential block. Will be automatically closed when returned object is destroyed.
        ArchiveBlock OpenSafeSequentialBlock(string name) { return OpenBlock(name, 0, true, ArchiveBlockType.Sequential); }
        /// Open safe Unordered block. Will be automatically closed when returned object is destroyed.
        ArchiveBlock OpenSafeUnorderedBlock(string name) { return OpenBlock(name, 0, true, ArchiveBlockType.Unordered); }
    }
}
