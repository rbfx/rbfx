import argparse
import json
import logging
import os

from clang_json.parser import ClangJsonAstParser
from passes.constants import DefineConstantsPass
from passes.enums import DefineEnumValuesPass
from passes.events import DefineEventsPass
from passes.properties import DiscoverInterfacesPass, DefinePropertiesPass
from passes.refcounted import DefineRefCountedPass
from passes.trim import TrimNodesPass
from writer import InterfaceWriter


def main():
    logging.basicConfig(level=logging.INFO)
    parser = argparse.ArgumentParser()
    parser.add_argument('--clang')
    parser.add_argument('--save-ast')
    parser.add_argument('--save-pp')
    parser.add_argument('--load-pp')
    parser.add_argument('--load-ast')
    parser.add_argument('--output')
    parser.add_argument('options')
    parser.add_argument('all_file')
    args = parser.parse_args()

    with open(args.options) as fp:
        compiler_params = fp.read().split('\n')
    compiler_params.append('-std=c++17')

    parser = ClangJsonAstParser(args.all_file, compiler_params, args.clang)
    if args.load_pp:
        logging.info(f'Load {args.load_pp}')
        parser._source_code = open(args.load_pp, 'rb').read()
    else:
        logging.info(f'Preprocess {args.all_file}')
        parser.preprocess()

    if args.load_ast:
        logging.info(f'Load {args.load_ast}')
        parser._json_ast = json.load(open(args.load_ast))
    else:
        logging.info(f'Parse {args.all_file}')
        parser.parse_json_ast()

    # Parse json ast and create a convenient object tree.
    root_node = parser.create_ast()

    # Truncate pregenerated files
    for file_name in os.listdir(args.output):
        os.truncate(os.path.join(args.output, file_name), 0)

    # Run passes on AST
    logging.info(f'Run AST passes')
    writer = InterfaceWriter(os.path.join(args.output))
    bind_lang = 'cs'
    try:
        passes = [
            TrimNodesPass(parser, writer, bind_lang),
            DefineConstantsPass(parser, writer, bind_lang),
            DefineRefCountedPass(parser, writer, bind_lang),
            DefineEnumValuesPass(parser, writer, bind_lang),
            DiscoverInterfacesPass(parser, writer, bind_lang),
            DefinePropertiesPass(parser, writer, bind_lang),
            DefineEventsPass(parser, writer, bind_lang),
        ]
        for pass_instance in passes:
            pass_instance.on_start()
            pass_instance.walk_ast(root_node)
            pass_instance.on_finish()
    finally:
        writer.close()

    # Save AST or preprocessed source code (for debugging)
    if args.save_ast is not None:
        logging.info(f'Save {args.save_ast}')
        parser.save_ast(args.save_ast)
    if args.save_pp:
        logging.info(f'Save {args.save_pp}')
        parser.save_preprocessed_source_code(args.save_pp)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
    except Exception as e:
        logging.exception(e)
