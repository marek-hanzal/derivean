import {type FC} from "react";
import {Button}  from "./Button";

export namespace ActionIcon {
    export interface Props extends Button.Props {
    }
}

export const ActionIcon: FC<ActionIcon.Props> = (
    {
        children,
        cn = [],
        ...props
    }
) => {
    return <Button
        variant={"subtle"}
        cn={[
            "p-1",
            ...cn,
        ]}
        {...props}
    >
        {children}
    </Button>;
};
