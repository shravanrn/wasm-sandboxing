class WasmSandbox
{
    void* lib;
public:
    static WasmSandbox* createSandbox(const char* path);
    void* symbolLookup(const char* name);
};