import {cn} from "@use-pico/utils";
import {
    type FC,
    type PropsWithChildren,
    type ReactNode
}           from "react";

const twSize = {
    "xs": "text-xs px-3 py-2 font-medium",
    "sm": "text-sm px-3 py-2 font-medium",
    "md": "text-md px-4 py-2 font-medium",
    "lg": "text-lg px-5 py-2",
    "xl": "text-xl px-6 py-3",
};
type twSize = typeof twSize;

export namespace Button {
    export type Props = PropsWithChildren<{
        leftSection?: ReactNode;
        size?: Size;
        onClick?: () => void;
    }>;

    export type Size = keyof twSize;
}

export const Button: FC<Button.Props> = (
    {
        size = "md",
        children,
    }
) => {
    return <button
        type={"button"}
        className={cn(`
            border-1
            rounded-md
            bg-primary-500
            hover:bg-primary-600
            text-white
        `, size ? twSize[size] : undefined)}
    >
        {children}
    </button>;
};
