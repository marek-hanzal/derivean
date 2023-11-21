import {type FC} from "react";
import {Text}    from "./Text";

const twOrder = {
    1: "text-4xl font-bold text-zinc-900",
    2: "text-3xl font-bold text-zinc-800",
    3: "text-2xl font-bold text-zinc-700",
    4: "text-xl font-semibold text-zinc-600",
    5: "text-lg text-zinc-500",
    6: "text-base text-zinc-500",
};
type twOrder = typeof twOrder;

export namespace Title {
    export interface Props extends Text.Props {
        order?: keyof twOrder;
    }
}

export const Title: FC<Title.Props> = (
    {
        order = 1,
        cn,
        ...props
    }
) => {
    return <Text
        cn={[
            twOrder[order],
            ...cn || [],
        ]}
        {...props}
    />;
};
