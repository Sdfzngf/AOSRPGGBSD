module;

export module Engine.Utils.Data.DataEntry.EntryType;

export namespace Engine {
    namespace Utils {
        namespace Data {
            enum class EntryType{
                Binary=0,
                String=1,
                List=2,
                Script=3
            };
        }
    }
}
