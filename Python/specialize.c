
#include "Python.h"
#include "pycore_code.h"
#include "opcode.h"


/* We layout the quickened data as a bi-directional array:
 * Instructions upwards, cache entries downwards.
 * first_instr is aligned to a SpecializedCacheEntry.
 * The nth instruction is located at first_instr[n]
 * The nth cache is located at ((SpecializedCacheEntry *)first_instr)[-1-n]
 * The first (index 0) cache entry is reserved for the count, to enable finding
 * the first instruction from the base pointer.
 * The cache_count argument must include space for the count.
 * We use the SpecializedCacheOrInstruction union to refer to the data
 * to avoid type punning.

 Layout of quickened data, each line 8 bytes:

 <cache_count>                              <---- co->co_quickened
 <cache N-1>
 <cache N-2>
 ...
 <cache 0>
 <instr 0> <instr 1> <instr 2> <instr 3>    <--- co->co_first_instr
 <instr 4> <instr 5> <instr 6> <instr 7>
 ...
 <instr N-1>
*/

static SpecializedCacheOrInstruction *
allocate(int cache_count, int instruction_count)
{
    assert(sizeof(SpecializedCacheOrInstruction) == 2*sizeof(int32_t));
    assert(sizeof(SpecializedCacheEntry) == 2*sizeof(int32_t));
    assert(cache_count > 0);
    assert(instruction_count > 0);
    int count = cache_count + (instruction_count + INSTRUCTIONS_PER_ENTRY -1)/INSTRUCTIONS_PER_ENTRY;
    SpecializedCacheOrInstruction *array = (SpecializedCacheOrInstruction *)
        PyMem_Malloc(sizeof(SpecializedCacheOrInstruction) * count);
    if (array == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    array[0].entry.zero.cache_count = cache_count;
    return array;
}

static int
get_cache_count(SpecializedCacheOrInstruction *quickened) {
    return quickened[0].entry.zero.cache_count;
}

/* Map from opcode to adaptive opcode.
  Values of zero are ignored. */
static uint8_t adaptive_opcodes[256] = { 0 };

/* The number of cache entries required for a "family" of instructions. */
static uint8_t cache_requirements[256] = { 0 };

/* Return the oparg for the cache_offset and instruction index,
 * or -1 if no cache is needed or possible.
 * The cache_offset is updated as a side-effect */
static int
oparg_from_instruction_and_update_offset(int index, int opcode, int original_oparg, int *cache_offset) {
    uint8_t need = cache_requirements[opcode];
    if (need == 0) {
        return original_oparg;
    }
    assert(adaptive_opcodes[opcode] != 0);
    int oparg = oparg_from_offset_and_index(*cache_offset, index);
    assert(*cache_offset == offset_from_oparg_and_index(oparg, index));
    if (oparg < 0) {
        *cache_offset = index/2;
        oparg = 0;
    }
    else if (oparg > 255) {
        return -1;
    }
    *cache_offset += need;
    return oparg;
}

static int
entries_needed(_Py_CODEUNIT *code, int len)
{
    int cache_offset = 0;
    int previous_opcode = -1;
    for (int i = 0; i < len; i++) {
        uint8_t opcode = _Py_OPCODE(code[i]);
        if (previous_opcode != EXTENDED_ARG) {
            oparg_from_instruction_and_update_offset(i, opcode, 0, &cache_offset);
        }
        previous_opcode = opcode;
    }
    return cache_offset+1;
}


static inline _Py_CODEUNIT *
first_instruction(SpecializedCacheOrInstruction *quickened)
{
    return &quickened[get_cache_count(quickened)].code[0];
}

static void
optimize(SpecializedCacheOrInstruction *quickened, int len)
{
    _Py_CODEUNIT *instructions = first_instruction(quickened);
    int cache_offset = 0;
    int previous_opcode = -1;
    for(int i = 0; i < len; i++) {
        int opcode = _Py_OPCODE(instructions[i]);
        int oparg = _Py_OPARG(instructions[i]);
        uint8_t adaptive_opcode = adaptive_opcodes[opcode];
        if (adaptive_opcode && previous_opcode != EXTENDED_ARG) {
            oparg = oparg_from_instruction_and_update_offset(i, opcode, oparg, &cache_offset);
            if (oparg >= 0) {
                instructions[i] = _Py_MAKECODEUNIT(adaptive_opcode, oparg);
                previous_opcode = adaptive_opcode;
            }
            else {
                previous_opcode = opcode;
            }
        }
        else {
            /* Super instructions don't use the cache, so no need to update the offset. */
            switch (opcode) {
                /* Insert superinstructions here
                 E.g.
                case LOAD_FAST:
                    if (previous_opcode == LOAD_FAST)
                        instructions[i-1] = _Py_MAKECODEUNIT(LOAD_FAST__LOAD_FAST, oparg);
                 */
            }
            previous_opcode = opcode;
        }
    }
    assert(cache_offset+1 == get_cache_count(quickened));
}

int
_Py_Quicken(PyCodeObject *code) {
    if (code->co_quickened) {
        return 0;
    }
    Py_ssize_t size = PyBytes_GET_SIZE(code->co_code);
    int instr_count = (int)(size/sizeof(_Py_CODEUNIT));
    if (instr_count > MAX_SIZE_TO_QUICKEN) {
        code->co_warmup = QUICKENING_WARMUP_COLDEST;
        return 0;
    }
    int entry_count = entries_needed(code->co_firstinstr, instr_count);
    SpecializedCacheOrInstruction *quickened = allocate(entry_count, instr_count);
    if (quickened == NULL) {
        return -1;
    }
    _Py_CODEUNIT *new_instructions = first_instruction(quickened);
    memcpy(new_instructions, code->co_firstinstr, size);
    optimize(quickened, instr_count);
    code->co_quickened = quickened;
    code->co_firstinstr = new_instructions;
    return 0;
}

