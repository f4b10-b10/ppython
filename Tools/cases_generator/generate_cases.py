"""Generate the main interpreter switch."""

# Write the cases to generated_cases.c.h, which is #included in ceval.c.

# TODO: Reuse C generation framework from deepfreeze.py?

import argparse
import io
import os
import re
import sys

import parser
from parser import InstDef  # TODO: Use parser.InstDef

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-i", "--input", type=str, default="Python/bytecodes.c")
arg_parser.add_argument("-o", "--output", type=str, default="Python/generated_cases.c.h")
arg_parser.add_argument("-c", "--compare", action="store_true")
arg_parser.add_argument("-q", "--quiet", action="store_true")


def eopen(filename: str, mode: str = "r"):
    if filename == "-":
        if "r" in mode:
            return sys.stdin
        else:
            return sys.stdout
    return open(filename, mode)


def parse_cases(
    src: str, filename: str|None = None
) -> tuple[list[InstDef], list[parser.Super], list[parser.Family]]:
    psr = parser.Parser(src, filename=filename)
    instrs: list[InstDef] = []
    supers: list[parser.Super] = []
    families: list[parser.Family] = []
    while not psr.eof():
        if inst := psr.inst_def():
            assert inst.block
            instrs.append(inst)
        elif sup := psr.super_def():
            supers.append(sup)
        elif fam := psr.family_def():
            families.append(fam)
        else:
            raise psr.make_syntax_error(f"Unexpected token")
    return instrs, supers, families


def always_exits(block: parser.Block) -> bool:
    text = block.text
    lines = text.splitlines()
    while lines and not lines[-1].strip():
        lines.pop()
    if not lines or lines[-1].strip() != "}":
        return False
    lines.pop()
    if not lines:
        return False
    line = lines.pop().rstrip()
    # Indent must match exactly (TODO: Do something better)
    if line[:12] != " "*12:
        return False
    line = line[12:]
    return line.startswith(("goto ", "return ", "DISPATCH", "GO_TO_", "Py_UNREACHABLE()"))


def write_cases(f: io.TextIOBase, instrs: list[InstDef], supers: list[parser.Super]):
    predictions: set[str] = set()
    for inst in instrs:
        assert inst.block is not None
        for target in re.findall(r"(?:PREDICT|GO_TO_INSTRUCTION)\((\w+)\)", inst.block.text):
            predictions.add(target)
    indent = "        "
    f.write(f"// This file is generated by {os.path.relpath(__file__)}\n")
    f.write("// Do not edit!\n")
    instr_index: dict[str, InstDef] = {}
    for instr in instrs:
        assert isinstance(instr, InstDef)
        instr_index[instr.name] = instr
        f.write(f"\n{indent}TARGET({instr.name}) {{\n")
        if instr.name in predictions:
            f.write(f"{indent}    PREDICTED({instr.name});\n")
        # TODO: Is it better to count forward or backward?
        for i, input in enumerate(reversed(instr.inputs or ()), 1):
            f.write(f"{indent}    PyObject *{input} = PEEK({i});\n")
        for output in instr.outputs or ():
            f.write(f"{indent}    PyObject *{output};\n")
        # input = ", ".join(instr.inputs)
        # output = ", ".join(instr.outputs)
        # f.write(f"{indent}    // {input} -- {output}\n")
        assert instr.block is not None
        blocklines = instr.block.text.splitlines(True)
        # Remove blank lines from ends
        while blocklines and not blocklines[0].strip():
            blocklines.pop(0)
        while blocklines and not blocklines[-1].strip():
            blocklines.pop()
        # Remove leading '{' and trailing '}'
        assert blocklines and blocklines[0].strip() == "{"
        assert blocklines and blocklines[-1].strip() == "}"
        blocklines.pop()
        blocklines.pop(0)
        # Remove trailing blank lines
        while blocklines and not blocklines[-1].strip():
            blocklines.pop()
        # Write the body
        ninputs = len(instr.inputs or ())
        for line in blocklines:
            if m := re.match(r"(\s*)ERROR_IF\(([^,]+), (\w+)\);\s*$", line):
                space, cond, label = m.groups()
                # ERROR_IF() must remove the inputs from the stack.
                # The code block is responsible for DECREF()ing them.
                if ninputs:
                    f.write(f"{space}if ({cond}) {{ STACK_SHRINK({ninputs}); goto {label}; }}\n")
                else:
                    f.write(f"{space}if ({cond}) {{ goto {label}; }}\n")
            else:
                f.write(line)
        noutputs = len(instr.outputs or ())
        diff = noutputs - ninputs
        if diff > 0:
            f.write(f"{indent}    STACK_GROW({diff});\n")
        elif diff < 0:
            f.write(f"{indent}    STACK_SHRINK({-diff});\n")
        for i, output in enumerate(reversed(instr.outputs or ()), 1):
            f.write(f"{indent}    POKE({i}, {output});\n")
        assert instr.block
        if not always_exits(instr.block):
            f.write(f"{indent}    DISPATCH();\n")
        # Write trailing '}'
        f.write(f"{indent}}}\n")

    for sup in supers:
        assert isinstance(sup, parser.Super)
        components = [instr_index[name] for name in sup.ops]
        f.write(f"\n{indent}TARGET({sup.name}) {{\n")
        for i, instr in enumerate(components):
            if i > 0:
                f.write(f"{indent}    NEXTOPARG();\n")
                f.write(f"{indent}    next_instr++;\n")
            text = instr.block.to_text(-4)
            textlines = text.splitlines(True)
            textlines = [line for line in textlines if not line.strip().startswith("PREDICTED(")]
            text = "".join(textlines)
            f.write(f"{indent}    {text.strip()}\n")
        f.write(f"{indent}    DISPATCH();\n")
        f.write(f"{indent}}}\n")


def main():
    args = arg_parser.parse_args()
    with eopen(args.input) as f:
        srclines = f.read().splitlines()
    begin = srclines.index("// BEGIN BYTECODES //")
    end = srclines.index("// END BYTECODES //")
    src = "\n".join(srclines[begin+1 : end])
    instrs, supers, families = parse_cases(src, filename=args.input)
    ninstrs = nsupers = nfamilies = 0
    if not args.quiet:
        ninstrs = len(instrs)
        nsupers = len(supers)
        nfamilies = len(families)
        print(
            f"Read {ninstrs} instructions, {nsupers} supers, "
            f"and {nfamilies} families from {args.input}",
            file=sys.stderr,
        )
    with eopen(args.output, "w") as f:
        write_cases(f, instrs, supers)
    if not args.quiet:
        print(
            f"Wrote {ninstrs + nsupers} instructions to {args.output}",
            file=sys.stderr,
        )


if __name__ == "__main__":
    main()
