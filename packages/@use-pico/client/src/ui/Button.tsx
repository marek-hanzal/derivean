import {
    type FC,
    type PropsWithChildren,
    type ReactNode
} from "react";

export namespace Button {
    export type Props = PropsWithChildren<{
        leftSection?: ReactNode;
        onClick?: () => void;
    }>;
}

export const Button: FC<Button.Props> = (
    {
        children,
    }
) => {
    return <button
        type={"button"}
        className={`
            border-1
            border-b-blue-600 
            rounded-md 
            bg-amber-200
            px-8
        `}

    >
        {children}
    </button>;
};
