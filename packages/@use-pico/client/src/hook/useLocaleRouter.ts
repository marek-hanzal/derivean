"use client";

import {
    useParams,
    useRouter
}                        from "next/navigation";
import {type IHrefProps} from "../api/IHrefProps";
import {linkTo}          from "../tools/linkTo";

/**
 * Use router with connected locale; it looks into params for "locale" parameter.
 *
 * If not found, it tries to detect current locale.
 */
export const useLocaleRouter = () => {
    const router = useRouter();
    const {locale} = useParams() as unknown as {
        locale: string
    };
    /**
     * Mimic original next.js router, override push
     */
    return {
        locale,
        push: (
                  {
                      href,
                      query
                  }: IHrefProps
              ) => {
            return router.push(linkTo({
                href: `/${locale ?? ""}${href}`,
                query,
            }));
        },
    } as const;
};
