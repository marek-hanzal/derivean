import {
    ItemInline,
    ItemUI
}                                          from "@derivean/item";
import {t}                                 from "@use-pico/i18n";
import {
    Group,
    NativeBreadcrumbs,
    Text
}                                          from "@use-pico/ui";
import {type FC}                           from "react";
import {BuildingConstructionRequirementUI} from "./BuildingConstructionRequirementUI";

export namespace BuildingConstructionRequirement {
    export interface Props {
        buildingId: string;
    }
}

export const BuildingConstructionRequirement: FC<BuildingConstructionRequirement.Props> = (
    {
        buildingId,
    }
) => {
    return <BuildingConstructionRequirementUI.Collection
        query={{
            where: {
                buildingId,
            }
        }}
        WithSuccess={({entities}) => {
            return <NativeBreadcrumbs
                separator={"&"}
                separatorMargin={4}
            >
                {entities.length > 0 && entities.map(requirement => <ItemUI.Fetch
                    key={requirement.id}
                    override={requirement.itemId}
                    WithSuccess={({entity}) => <Group gap={4}>
                        <ItemInline entity={entity}/>
                        <Text fw={500}>x{requirement.amount}</Text>
                    </Group>}
                />)}
                {!entities.length && <Text c={"dimmed"}>({t()`Building without requirements`})</Text>}
            </NativeBreadcrumbs>;
        }}
    />;
};
