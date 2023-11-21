"use client";

import {
    type FC,
    type ReactNode
}                        from "react";
import {type IHrefProps} from "../api/IHrefProps";
import {LinkLockStore}   from "../store/LinkLockStore";
import {Button}          from "./Button";
import {LocaleLink}      from "./LocaleLink";

export namespace ButtonLink {
    export interface Props extends Button.Props {
        icon?: ReactNode;
        href: IHrefProps | string;
        label?: ReactNode;
        withLocale?: boolean;
        target?: LocaleLink.Props["target"];
    }
}

export const ButtonLink: FC<ButtonLink.Props> = (
    {
        icon,
        href,
        disabled = false,
        withLocale = true,
        target,
        ...props
    }
) => {
    const isLock = LinkLockStore.useSelector$(({isLock}) => isLock);
    const buttonProps: Button.Props = {
        variant:     "subtle",
        cn:          [
            "text-sky-500 hover:text-sky-600",
        ],
        size:        "compact-md",
        leftSection: icon,
        ...props,
    };

    if (!href || disabled || isLock) {
        return <Button
            disabled
            {...buttonProps}
        />;
    }
    return <LocaleLink
        href={href}
        withLocale={withLocale}
        target={target}
    >
        <Button
            {...buttonProps}
        />
    </LocaleLink>;
};
