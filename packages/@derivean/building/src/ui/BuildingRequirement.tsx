import {
    ItemInline,
    ItemUI
}                              from "@derivean/item";
import {
    Group,
    Nav,
    Text
}                              from "@use-pico/client";
import {t}                     from "@use-pico/translator";
import {type FC}               from "react";
import {BuildingRequirementUI} from "./BuildingRequirementUI";

export namespace BuildingRequirement {
    export interface Props {
        buildingId: string;
    }
}

export const BuildingRequirement: FC<BuildingRequirement.Props> = (
    {
        buildingId,
    }
) => {
    return <BuildingRequirementUI.Collection
        query={{
            where: {
                buildingId,
            }
        }}
        WithSuccess={({entities}) => {
            return entities.length > 0 ? <Nav
                separator={"&"}
                separatorMargin={4}
                items={entities.map(requirement => <ItemUI.Fetch
                    key={requirement.id}
                    override={requirement.itemId}
                    WithSuccess={({entity}) => <Group gap={4}>
                        <ItemInline entity={entity}/>
                        <Text fw={500}>x{requirement.amount}</Text>
                    </Group>}
                />)}
            /> : <Text c={"dimmed"}>({t()`Building without requirements`})</Text>;
        }}
    />;
};
