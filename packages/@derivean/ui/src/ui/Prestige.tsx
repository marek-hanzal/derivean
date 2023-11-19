import {
    IconStar,
    IconStarFilled
}                from "@tabler/icons-react";
import {
    type CommonProps,
    Icon,
    Rating
}                from "@use-pico/client";
import {type FC} from "react";

const colors: Record<number, CommonProps.Color> = {
    1: "green",
    2: "blue",
    3: "yellow",
    4: "orange",
    5: "violet",
    6: "grape",
} as const;

export namespace Prestige {
    export interface Props {
        prestige: number;
    }
}

export const Prestige: FC<Prestige.Props> = (
    {
        prestige,
    }
) => {
    return <Rating
        defaultValue={prestige}
        readOnly
        size={"md"}
        emptySymbol={value => <Icon color={colors[value] ?? "green"} Icon={IconStar}/>}
        fullSymbol={value => <Icon color={colors[value] ?? "green"} Icon={IconStarFilled}/>}
        count={6}
    />;
};
