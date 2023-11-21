import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {tailwindify}      from "../tools/tailwindify";

export namespace Container {
    export type Props = PropsWithChildren<CommonProps>;
}

export const Container: FC<Container.Props> = (
    {
        children,
        ...props
    }
) => {
    const {cn, $props} = tailwindify(props);

    return <div
        className={cn([
            "container mx-auto"
        ])}
        {...$props}
    >
        {children}
    </div>;
};
