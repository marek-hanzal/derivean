import {
    diffOf,
    isEmpty,
    isString
}                        from "@use-pico/utils";
import buildUrl          from "build-url-ts";
import {
    compile,
    match
}                        from "path-to-regexp";
import {type IHrefProps} from "../api/IHrefProps";

/**
 * Quite a clever url link generator: takes URL template (like /foo/[id]) with query params and expands
 * all sections + generates query params.
 */
export const linkTo = <
    TPath extends string,
>(
    props: IHrefProps<TPath> | TPath
) => {
    if (isString(props)) {
        return props;
    }
    const {
        query,
        href
    } = props;
    const $query = query || {};
    const link = href.replace(/\[(.*?)\]/g, ":$1").replace(/{(.*?)}/g, ":$1");
    const compiled = compile(link)($query);
    const matched = match(link)(compiled);
    const queryParams = diffOf(
        Object.keys($query),
        Object.keys(matched ? matched.params : {}),
    ).reduce((prev, current) => {
        return {
            ...prev,
            [current]: (query as any)[current],
        };
    }, {});

    return buildUrl({
        path:        compiled,
        queryParams: isEmpty(queryParams) ? undefined : queryParams,
    });
};
