from pathlib import Path
from typing import TextIO

from analyzer import (
    Instruction,
    Uop,
    Properties,
    StackItem,
    analysis_error,
)
from cwriter import CWriter
from typing import Callable, Mapping, TextIO, Iterator
from lexer import Token
from stack import Stack


ROOT = Path(__file__).parent.parent.parent
DEFAULT_INPUT = (ROOT / "Python/bytecodes.c").absolute().as_posix()


def root_relative_path(filename: str) -> str:
    try:
        return Path(filename).absolute().relative_to(ROOT).as_posix()
    except ValueError:
        # Not relative to root, just return original path.
        return filename


def type_and_null(var: StackItem) -> tuple[str, str]:
    if var.type:
        return var.type, "NULL"
    elif var.is_array():
        return "_PyStackRef *", "NULL"
    else:
        return "_PyStackRef", "PyStackRef_NULL"


def write_header(
    generator: str, sources: list[str], outfile: TextIO, comment: str = "//"
) -> None:
    outfile.write(
        f"""{comment} This file is generated by {root_relative_path(generator)}
{comment} from:
{comment}   {", ".join(root_relative_path(src) for src in sources)}
{comment} Do not edit!
"""
    )


def emit_to(out: CWriter, tkn_iter: Iterator[Token], end: str) -> None:
    parens = 0
    for tkn in tkn_iter:
        if tkn.kind == end and parens == 0:
            return
        if tkn.kind == "LPAREN":
            parens += 1
        if tkn.kind == "RPAREN":
            parens -= 1
        out.emit(tkn)

ReplacementFunctionType = Callable[
    [Token, Iterator[Token], Uop, Stack, Instruction | None], None
]

class Emitter:

    out: CWriter
    _replacers: dict[str, ReplacementFunctionType]

    def __init__(self, out: CWriter):
        self._replacers = {
            "EXIT_IF": self.exit_if,
            "DEOPT_IF": self.deopt_if,
            "ERROR_IF": self.error_if,
            "ERROR_NO_POP": self.error_no_pop,
            "DECREF_INPUTS": self.decref_inputs,
            "CHECK_EVAL_BREAKER": self.check_eval_breaker,
            "SYNC_SP": self.sync_sp,
            "PyStackRef_FromPyObjectNew": self.py_stack_ref_from_py_object_new,
        }
        self.out = out

    def deopt_if(
        self,
        tkn: Token,
        tkn_iter: Iterator[Token],
        uop: Uop,
        unused: Stack,
        inst: Instruction | None,
    ) -> None:
        self.out.emit_at("DEOPT_IF", tkn)
        self.out.emit(next(tkn_iter))
        emit_to(self.out, tkn_iter, "RPAREN")
        next(tkn_iter)  # Semi colon
        self.out.emit(", ")
        assert inst is not None
        assert inst.family is not None
        self.out.emit(inst.family.name)
        self.out.emit(");\n")

    exit_if = deopt_if

    def error_if(
        self,
        tkn: Token,
        tkn_iter: Iterator[Token],
        uop: Uop,
        stack: Stack,
        inst: Instruction | None,
    ) -> None:
        self.out.emit_at("if ", tkn)
        self.out.emit(next(tkn_iter))
        emit_to(self.out, tkn_iter, "COMMA")
        label = next(tkn_iter).text
        next(tkn_iter)  # RPAREN
        next(tkn_iter)  # Semi colon
        self.out.emit(") ")
        c_offset = stack.peek_offset()
        try:
            offset = -int(c_offset)
        except ValueError:
            offset = -1
        if offset > 0:
            self.out.emit(f"goto pop_{offset}_")
            self.out.emit(label)
            self.out.emit(";\n")
        elif offset == 0:
            self.out.emit("goto ")
            self.out.emit(label)
            self.out.emit(";\n")
        else:
            self.out.emit("{\n")
            stack.flush_locally(self.out)
            self.out.emit("goto ")
            self.out.emit(label)
            self.out.emit(";\n")
            self.out.emit("}\n")

    def error_no_pop(
        self,
        tkn: Token,
        tkn_iter: Iterator[Token],
        uop: Uop,
        stack: Stack,
        inst: Instruction | None,
    ) -> None:
        next(tkn_iter)  # LPAREN
        next(tkn_iter)  # RPAREN
        next(tkn_iter)  # Semi colon
        self.out.emit_at("goto error;", tkn)

    def decref_inputs(
        self,
        tkn: Token,
        tkn_iter: Iterator[Token],
        uop: Uop,
        stack: Stack,
        inst: Instruction | None,
    ) -> None:
        next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        self.out.emit_at("", tkn)
        for var in uop.stack.inputs:
            if var.name == "unused" or var.name == "null" or var.peek:
                continue
            if var.size:
                self.out.emit(f"for (int _i = {var.size}; --_i >= 0;) {{\n")
                self.out.emit(f"PyStackRef_CLOSE({var.name}[_i]);\n")
                self.out.emit("}\n")
            elif var.condition:
                if var.condition == "1":
                    self.out.emit(f"PyStackRef_CLOSE({var.name});\n")
                elif var.condition != "0":
                    self.out.emit(f"PyStackRef_XCLOSE({var.name});\n")
            else:
                self.out.emit(f"PyStackRef_CLOSE({var.name});\n")


    def sync_sp(
        self,
        tkn: Token,
        tkn_iter: Iterator[Token],
        uop: Uop,
        stack: Stack,
        inst: Instruction | None,
    ) -> None:
        next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        stack.flush(self.out)


    def check_eval_breaker(
        self,
        tkn: Token,
        tkn_iter: Iterator[Token],
        uop: Uop,
        stack: Stack,
        inst: Instruction | None,
    ) -> None:
        next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        if not uop.properties.ends_with_eval_breaker:
            self.out.emit_at("CHECK_EVAL_BREAKER();", tkn)

    def py_stack_ref_from_py_object_new(
        self,
        tkn: Token,
        tkn_iter: Iterator[Token],
        uop: Uop,
        stack: Stack,
        inst: Instruction | None,
    ) -> None:
        self.out.emit(tkn)
        emit_to(self.out, tkn_iter, "SEMI")
        self.out.emit(";\n")

        target = uop.deferred_refs[tkn]
        if target is None:
            # An assignment we don't handle, such as to a pointer or array.
            return

        # Flush the assignment to the stack.  Note that we don't flush the
        # stack pointer here, and instead are currently relying on initializing
        # unused portions of the stack to NULL.
        stack.flush_single_var(self.out, target)


    def emit_tokens(
        self,
        uop: Uop,
        stack: Stack,
        inst: Instruction | None,
    ) -> None:
        tkns = uop.body[1:-1]
        if not tkns:
            return
        tkn_iter = iter(tkns)
        self.out.start_line()
        for tkn in tkn_iter:
            if tkn.kind == "IDENTIFIER" and tkn.text in self._replacers:
                self._replacers[tkn.text](tkn, tkn_iter, uop, stack, inst)
            else:
                self.out.emit(tkn)

    def emit(self, txt: str | Token) -> None:
        self.out.emit(txt)

def cflags(p: Properties) -> str:
    flags: list[str] = []
    if p.oparg:
        flags.append("HAS_ARG_FLAG")
    if p.uses_co_consts:
        flags.append("HAS_CONST_FLAG")
    if p.uses_co_names:
        flags.append("HAS_NAME_FLAG")
    if p.jumps:
        flags.append("HAS_JUMP_FLAG")
    if p.has_free:
        flags.append("HAS_FREE_FLAG")
    if p.uses_locals:
        flags.append("HAS_LOCAL_FLAG")
    if p.eval_breaker:
        flags.append("HAS_EVAL_BREAK_FLAG")
    if p.deopts:
        flags.append("HAS_DEOPT_FLAG")
    if p.side_exit:
        flags.append("HAS_EXIT_FLAG")
    if not p.infallible:
        flags.append("HAS_ERROR_FLAG")
    if p.error_without_pop:
        flags.append("HAS_ERROR_NO_POP_FLAG")
    if p.escapes:
        flags.append("HAS_ESCAPES_FLAG")
    if p.pure:
        flags.append("HAS_PURE_FLAG")
    if p.oparg_and_1:
        flags.append("HAS_OPARG_AND_1_FLAG")
    if flags:
        return " | ".join(flags)
    else:
        return "0"
