import {
    ItemFetch,
    ItemInline
}                                      from "@derivean/item";
import {
    Nav,
    Text
}                                      from "@use-pico/client";
import {t}                             from "@use-pico/translator";
import {type FC}                       from "react";
import {BuildingRequirementCollection} from "./BuildingRequirementComponents";

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
    return <BuildingRequirementCollection
        query={{
            where: {
                buildingId,
            }
        }}
        WithSuccess={({entities}) => {
            return entities.length > 0 ? <Nav
                separator={"&"}
                separatorMargin={"gap-4"}
                items={entities.map(requirement => ({
                    type:      "custom",
                    component: <ItemFetch
                                   key={requirement.id}
                                   override={requirement.itemId}
                                   WithSuccess={({entity}) => <div className={"flex items-center"}>
                                       <ItemInline entity={entity}/>
                                       <Text fw={500}>x{requirement.amount}</Text>
                                   </div>}
                               />,
                }))}
            /> : <Text c={"dimmed"}>({t`Building without requirements`})</Text>;
        }}
    />;
};
