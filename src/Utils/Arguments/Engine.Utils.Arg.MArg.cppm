/**
 * @brief Engine.Utils.Param.MParam.cppm，程序命令行参数的结构体。
 *
 */
module;

export module Engine.Utils.Arg.MArg;

export namespace Engine::Utils::Arg {
struct MArg {
    // 对应命令 --test-param1
    bool _test_param1 { false };
    // 对应命令 --test-param2
    bool _test_param2 { false };
    // 对应命令 --test-param2
    bool _test_param3 { false };
    // 对应命令 --dev-console
    bool _dev_console { false };
    // --help
    bool _help { false };
};
}
