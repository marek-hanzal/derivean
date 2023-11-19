import {
    type FC,
    type PropsWithChildren
}                         from "react";
import {type CommonProps} from "../api/CommonProps";
import {Flex}             from "./Flex";
import {FlexItem}         from "./Flex/FlexItem";

export namespace Center {
    export type Props = PropsWithChildren<CommonProps>;
}

export const Center: FC<Center.Props> = (
    {
        children,
        ...props
    }
) => {
    return <Flex
        align={"center"}
        {...props}
    >
        <FlexItem>
            {children}
        </FlexItem>
    </Flex>;
};
