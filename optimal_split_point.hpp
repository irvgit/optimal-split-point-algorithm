#pragma once

#if __cplusplus < 202302L
    #error out of date c++ version, compile with -stdc++=2c
#elif defined(__clang__) && __clang_major__ < 19
    #error out of date clang, compile with latest version
#elif !defined(__clang__) && defined(__GNUC__) && __GNUC__ < 14
    #error out of date g++, compile with latest version
#elif defined(_MSC_VER)
    #error msvc does not yet support the latest c++ features
#else

#include <algorithm>
#include <concepts>
#include <functional>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace osp {
    namespace detail {
        template <typename tp_type_t>
        struct is_subrange : std::false_type {};
        template <
            std::input_iterator                      tp_input_itereator_t,
            std::sentinel_for<tp_input_itereator_t> tp_sentinel_iterator_t,
            std::ranges::subrange_kind               tp_subrange_kind
        >
        struct is_subrange<std::ranges::subrange<
            tp_input_itereator_t,
            tp_sentinel_iterator_t,
            tp_subrange_kind
        >> : std::true_type {};
        template <typename tp_type_t>
        concept subrange = is_subrange<std::remove_cvref_t<tp_type_t>>::value;

        template <typename tp_type_t>
        struct is_pair : std::false_type {};
        template <
            typename tp_type1_t,
            typename tp_type2_t
        >
        struct is_pair<std::pair<
            tp_type1_t,
            tp_type2_t
        >> : std::true_type {};
        template <typename tp_type_t>
        concept pair = is_pair<std::remove_cvref_t<tp_type_t>>::value;

        template <typename tp_type_t>
        concept subrange_pair =
            pair<tp_type_t> &&
            subrange<typename std::remove_cvref_t<tp_type_t>::first_type> &&
            subrange<typename std::remove_cvref_t<tp_type_t>::second_type>;
    }

    namespace detail {
        struct to_split_point_fn {
            template <
                subrange tp_subrange1_t,
                subrange tp_subrange2_t
            >
            auto constexpr operator()[[nodiscard]] (
                tp_subrange1_t p_partition_left,
                tp_subrange2_t p_partition_right
            )
            const
            -> std::ranges::sentinel_t<tp_subrange1_t> {
                return std::ranges::end(p_partition_left);
            }
            template <subrange_pair tp_subrange_pair_t>
            auto constexpr operator()[[nodiscard]] (tp_subrange_pair_t&& p_partition)
            const
            -> std::ranges::sentinel_t<typename std::remove_cvref_t<tp_subrange_pair_t>::first_type> {
                return (*this)(
                    p_partition.first,
                    p_partition.second
                );
            }
        };
    }
    auto constexpr to_split_point = detail::to_split_point_fn{};
    
    namespace detail {
        struct to_partition_fn {
            template <
                std::input_iterator                     tp_input_iterator1_t,
                std::sentinel_for<tp_input_iterator1_t> tp_sentinel_iterator_t,
                std::input_iterator                     tp_input_iterator2_t
            >
            requires (
                std::sentinel_for<
                    tp_input_iterator2_t,
                    tp_sentinel_iterator_t
                >
            )
            auto constexpr operator()[[nodiscard]] (
                tp_input_iterator1_t   p_first,
                tp_sentinel_iterator_t p_last,
                tp_input_iterator2_t   p_split_point
            )
            const
            -> std::pair<
                std::ranges::subrange<
                    tp_input_iterator1_t,
                    tp_input_iterator2_t
                >,
                std::ranges::subrange<
                    tp_input_iterator2_t,
                    tp_sentinel_iterator_t
                >
            > {
                using return_type = std::pair<
                    std::ranges::subrange<
                        tp_input_iterator1_t,
                        tp_input_iterator2_t
                    >,
                    std::ranges::subrange<
                        tp_input_iterator2_t,
                        tp_sentinel_iterator_t
                    >
                >;
                return return_type{
                    typename return_type::first_type{
                        std::move(p_first),
                        p_split_point
                    },
                    typename return_type::second_type{
                        std::move(p_split_point),
                        std::move(p_last)
                    }
                };
            }
            template <
                std::ranges::input_range tp_input_range_t,
                std::input_iterator      tp_input_iterator_t
            >
            requires (
                std::sentinel_for<
                    tp_input_iterator_t,
                    std::ranges::sentinel_t<tp_input_range_t>
                >
            )
            auto constexpr operator()[[nodiscard]] (
                tp_input_range_t&&  p_range,
                tp_input_iterator_t p_split_point
            )
            const
            -> std::pair<
                std::ranges::subrange<
                    std::ranges::iterator_t<tp_input_range_t>,
                    tp_input_iterator_t
                >,
                std::ranges::subrange<
                    tp_input_iterator_t,
                    std::ranges::sentinel_t<tp_input_range_t>
                >
            > {
                return (*this)(
                    std::ranges::begin(p_range),
                    std::ranges::end(p_range),
                    std::move(p_split_point)
                );
            }
        };
    }
    auto constexpr to_partition = detail::to_partition_fn{};
    
    namespace detail {
        template <std::intmax_t tp_downscaling, bool tp_disable_backtracking>
        requires (std::cmp_greater_equal(tp_downscaling, 0))
        struct efficient_binary_split_point_fn {
            template <
                std::input_iterator                    tp_input_iterator_t,
                std::sentinel_for<tp_input_iterator_t> tp_sentinel_iterator_t,
                typename                               tp_cost_operation_t,
                typename                               tp_comp_operation_t  = std::ranges::less,
                typename                               tp_cost_projection_t = std::identity,
                typename                               tp_comp_projection_t = std::identity
            >
            requires (
                std::predicate<
                    tp_comp_operation_t,
                    std::invoke_result_t<
                        std::plus<>,
                        std::invoke_result_t<
                            tp_cost_operation_t,
                            std::invoke_result_t<
                                tp_cost_projection_t,
                                std::ranges::subrange<
                                    tp_input_iterator_t,
                                    tp_input_iterator_t
                                >
                            >
                        >,
                        std::invoke_result_t<
                            tp_cost_operation_t,
                            std::invoke_result_t<
                                tp_cost_projection_t,
                                std::ranges::subrange<
                                    tp_input_iterator_t,
                                    tp_sentinel_iterator_t
                                >
                            >
                        >
                    >,
                    std::invoke_result_t<
                        tp_cost_operation_t,
                        std::invoke_result_t<
                            tp_cost_projection_t,
                            std::ranges::subrange<
                                tp_input_iterator_t,
                                tp_sentinel_iterator_t
                            >
                        >
                    >
                >
            )
            auto constexpr operator()[[nodiscard]] (
                tp_input_iterator_t    p_first,
                tp_sentinel_iterator_t p_last,
                tp_cost_operation_t    p_cost_operation,
                tp_comp_operation_t    p_comp_operation  = {},
                tp_cost_projection_t   p_cost_projection = {},
                tp_comp_projection_t   p_comp_projection = {}
            )
            const
            -> tp_input_iterator_t {
                auto constexpr l_impl = [](auto&& p_left, auto&& p_middle, auto&& p_right, auto&& p_cost_op, auto&& p_comp_op, auto&& p_cost_proj, auto&& p_comp_proj, auto&& p_cost, auto&& p_result) {
                    if (auto l_sum = std::invoke(p_cost_op, std::invoke(p_cost_proj, std::ranges::subrange{p_left, p_middle})) + std::invoke(p_cost_op, std::invoke(p_cost_proj, std::ranges::subrange{p_middle, p_right})); std::invoke(p_comp_op, std::invoke(p_comp_proj, l_sum), p_cost)) {
                        p_result = std::move(p_middle);
                        p_cost   = std::move(l_sum);
                    }
                };
                if (p_first == p_last || std::ranges::next(p_first) == p_last)
                    return p_last;
                auto l_result       = p_last;
                auto l_current_cost = std::invoke(p_cost_operation, std::invoke(p_cost_projection, std::ranges::subrange{p_first, p_last}));
                for (auto l_iterator = std::ranges::next(p_first); l_iterator != p_last; std::ranges::advance(l_iterator, static_cast<std::iter_difference_t<tp_input_iterator_t>>(std::intmax_t{1} + tp_downscaling), p_last))
                    l_impl(p_first, l_iterator, p_last, p_cost_operation, p_comp_operation, p_cost_projection, p_comp_projection, l_current_cost, l_result);
                if constexpr (!tp_disable_backtracking && tp_downscaling && std::bidirectional_iterator<tp_input_iterator_t>)
                    if (l_result != p_last)
                        for (auto l_iterator = std::counted_iterator{std::reverse_iterator{std::ranges::prev(l_result)}, tp_downscaling}; l_iterator != std::default_sentinel; ++l_iterator)
                            l_impl(p_first, l_iterator.base().base(), p_last, p_cost_operation, p_comp_operation, p_cost_projection, p_comp_projection, l_current_cost, l_result);
                return l_result;
            }
            template <
                std::ranges::input_range tp_input_range_t,
                typename                 tp_cost_operation_t,
                typename                 tp_comp_operation_t  = std::ranges::less,
                typename                 tp_cost_projection_t = std::identity,
                typename                 tp_comp_projection_t = std::identity
            >
            requires (
                std::predicate<
                    tp_comp_operation_t,
                    std::invoke_result_t<
                        std::plus<>,
                        std::invoke_result_t<
                            tp_cost_operation_t,
                            std::invoke_result_t<
                                tp_cost_projection_t,
                                std::ranges::subrange<
                                    std::ranges::iterator_t<tp_input_range_t>,
                                    std::ranges::iterator_t<tp_input_range_t>
                                >
                            >
                        >,
                        std::invoke_result_t<
                            tp_cost_operation_t,
                            std::invoke_result_t<
                                tp_cost_projection_t,
                                std::ranges::subrange<
                                    std::ranges::iterator_t<tp_input_range_t>,
                                    std::ranges::sentinel_t<tp_input_range_t>
                                >
                            >
                        >
                    >,
                    std::invoke_result_t<
                        tp_cost_operation_t,
                        std::invoke_result_t<
                            tp_cost_projection_t,
                            std::ranges::subrange<
                                std::ranges::iterator_t<tp_input_range_t>,
                                std::ranges::sentinel_t<tp_input_range_t>
                            >
                        >
                    >
                >
            )
            auto constexpr operator()[[nodiscard]] (
                tp_input_range_t&&   p_range,
                tp_cost_operation_t  p_cost_operation,
                tp_comp_operation_t  p_comp_operation  = {},
                tp_cost_projection_t p_cost_projection = {},
                tp_comp_projection_t p_comp_projection = {}
            )
            const
            -> std::ranges::iterator_t<tp_input_range_t> {
                return (*this)(
                    std::ranges::begin(p_range),
                    std::ranges::end(p_range),
                    std::move(p_cost_operation),
                    std::move(p_comp_operation),
                    std::move(p_cost_projection),
                    std::move(p_comp_projection)
                );
            }
        };
    }
    template <std::intmax_t tp_downscaling = 0, bool tp_disable_backtracking = false>
    requires (std::cmp_greater_equal(tp_downscaling, 0))
    auto constexpr efficient_binary_split_point = detail::efficient_binary_split_point_fn<tp_downscaling, tp_disable_backtracking>{};
    auto constexpr optimal_binary_split_point   = efficient_binary_split_point<0>;    

    namespace detail {
        template <std::intmax_t tp_downscaling, bool tp_disable_backtracking>
        requires (std::cmp_greater_equal(tp_downscaling, 0))
        struct efficient_binary_split_fn {
            template <
                std::input_iterator                    tp_input_iterator_t,
                std::sentinel_for<tp_input_iterator_t> tp_sentinel_iterator_t,
                typename                               tp_cost_operation_t,
                typename                               tp_comp_operation_t  = std::ranges::less,
                typename                               tp_cost_projection_t = std::identity,
                typename                               tp_comp_projection_t = std::identity
            >
            requires (
                std::predicate<
                    tp_comp_operation_t,
                    std::invoke_result_t<
                        std::plus<>,
                        std::invoke_result_t<
                            tp_cost_operation_t,
                            std::invoke_result_t<
                                tp_cost_projection_t,
                                std::ranges::subrange<
                                    tp_input_iterator_t,
                                    tp_input_iterator_t
                                >
                            >
                        >,
                        std::invoke_result_t<
                            tp_cost_operation_t,
                            std::invoke_result_t<
                                tp_cost_projection_t,
                                std::ranges::subrange<
                                    tp_input_iterator_t,
                                    tp_sentinel_iterator_t
                                >
                            >
                        >
                    >,
                    std::invoke_result_t<
                        tp_cost_operation_t,
                        std::invoke_result_t<
                            tp_cost_projection_t,
                            std::ranges::subrange<
                                tp_input_iterator_t,
                                tp_sentinel_iterator_t
                            >
                        >
                    >
                >
            )
            auto constexpr operator()[[nodiscard]] (
                tp_input_iterator_t    p_first,
                tp_sentinel_iterator_t p_last,
                tp_cost_operation_t    p_cost_operation,
                tp_comp_operation_t    p_comp_operation  = {},
                tp_cost_projection_t   p_cost_projection = {},
                tp_comp_projection_t   p_comp_projection = {}
            )
            const
            -> std::pair<
                std::ranges::subrange<
                    tp_input_iterator_t,
                    tp_input_iterator_t
                >,
                std::ranges::subrange<
                    tp_input_iterator_t,
                    tp_sentinel_iterator_t
                >
            > {
                using return_type = std::pair<
                    std::ranges::subrange<
                        tp_input_iterator_t,
                        tp_input_iterator_t
                    >,
                    std::ranges::subrange<
                        tp_input_iterator_t,
                        tp_sentinel_iterator_t
                    >
                >;
                auto l_splt_point = efficient_binary_split_point<
                    tp_downscaling,
                    tp_disable_backtracking
                > (
                    p_first,
                    p_last,
                    std::move(p_cost_operation),
                    std::move(p_comp_operation),
                    std::move(p_cost_projection),
                    std::move(p_comp_projection)
                );
                return to_partition(
                    std::move(p_first),
                    std::move(p_last),
                    std::move(l_splt_point)
                );
            }
            template <
                std::ranges::input_range tp_input_range_t,
                typename                 tp_cost_operation_t,
                typename                 tp_comp_operation_t  = std::ranges::less,
                typename                 tp_cost_projection_t = std::identity,
                typename                 tp_comp_projection_t = std::identity
            >
            requires (
                std::predicate<
                    tp_comp_operation_t,
                    std::invoke_result_t<
                        std::plus<>,
                        std::invoke_result_t<
                            tp_cost_operation_t,
                            std::invoke_result_t<
                                tp_cost_projection_t,
                                std::ranges::subrange<
                                    std::ranges::iterator_t<tp_input_range_t>,
                                    std::ranges::iterator_t<tp_input_range_t>
                                >
                            >
                        >,
                        std::invoke_result_t<
                            tp_cost_operation_t,
                            std::invoke_result_t<
                                tp_cost_projection_t,
                                std::ranges::subrange<
                                    std::ranges::iterator_t<tp_input_range_t>,
                                    std::ranges::sentinel_t<tp_input_range_t>
                                >
                            >
                        >
                    >,
                    std::invoke_result_t<
                        tp_cost_operation_t,
                        std::invoke_result_t<
                            tp_cost_projection_t,
                            std::ranges::subrange<
                                std::ranges::iterator_t<tp_input_range_t>,
                                std::ranges::sentinel_t<tp_input_range_t>
                            >
                        >
                    >
                >
            )
            auto constexpr operator()[[nodiscard]] (
                tp_input_range_t&&   p_range,
                tp_cost_operation_t  p_cost_operation,
                tp_comp_operation_t  p_comp_operation  = {},
                tp_cost_projection_t p_cost_projection = {},
                tp_comp_projection_t p_comp_projection = {}
            )
            const
            -> std::pair<
                std::ranges::subrange<
                    std::ranges::iterator_t<tp_input_range_t>,
                    std::ranges::iterator_t<tp_input_range_t>
                >,
                std::ranges::subrange<
                    std::ranges::iterator_t<tp_input_range_t>,
                    std::ranges::sentinel_t<tp_input_range_t>
                >
            > {
                return (*this)(
                    std::ranges::begin(p_range),
                    std::ranges::end(p_range),
                    std::move(p_cost_operation),
                    std::move(p_comp_operation),
                    std::move(p_cost_projection),
                    std::move(p_comp_projection)
                );
            }
        };
    }
    template <std::intmax_t tp_downscaling = 0, bool tp_disable_backtracking = false>
    requires (std::cmp_greater_equal(tp_downscaling, 0))
    auto constexpr efficient_binary_split = detail::efficient_binary_split_fn<tp_downscaling, tp_disable_backtracking>{};
    auto constexpr optimal_binary_split   = efficient_binary_split<0>;
}

#endif
