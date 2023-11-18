import {type FC} from "react";
import {Button}  from "./Button";

export namespace ActionIcon {
    export interface Props extends Button.Props {
    }
}

export const ActionIcon: FC<ActionIcon.Props> = (
    {
        children,
        ...props
    }
) => {
    return <Button
        variant={"subtle"}
        className={"p-2"}
        {...props}
    >
        {children}
    </Button>;
};
