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

export const Button: FC<Button.Props> = () => {
    return "Button";
};
