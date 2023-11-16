import {
    IconStar,
    IconStarFilled
}                from "@tabler/icons-react";
import {
    type Color,
    Rating,
    WithIcon
}                from "@use-pico/ui";
import {type FC} from "react";

const colors: Record<number, Color> = {
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
        emptySymbol={value => <WithIcon color={colors[value] ?? "green"} icon={<IconStar/>}/>}
        fullSymbol={value => <WithIcon color={colors[value] ?? "green"} icon={<IconStarFilled/>}/>}
        count={6}
    />;
};
