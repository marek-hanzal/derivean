import {withBuildingRepository} from "@derivean/building";
import {BuildingMenu}           from "@derivean/manager";
import {
    Dependencies,
    Pipeline
}                               from "@derivean/producer";
import {container}              from "@derivean/server";
import {BuildingIcon}           from "@derivean/ui";
import {t}                      from "@use-pico/i18n";
import {
    Box,
    Breadcrumbs,
    HomeIcon,
    ListIcon,
    Page
}                               from "@use-pico/ui";

export namespace Index {
    export interface Props {
        params: {
            id: string;
        };
    }
}

export default async function Index({params: {id}}: Index.Props) {
    const building = await withBuildingRepository.use(container).withQuery.fetchOrThrow({
        where: {
            id,
        }
    });

    return <Page
        icon={<BuildingIcon/>}
        text={{
            header: t({values: building})`Building pipeline`,
        }}
        postfix={<Breadcrumbs
            items={[
                {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
                {
                    type:  "link",
                    href:  "/manager/building/list",
                    label: t()`Building list`,
                    icon:  <ListIcon/>,
                },
            ]}
        />}
        append={<BuildingMenu
            buildingId={building.id}
            active={["/manager/building/[id]/pipeline"]}
        />}
    >
        <Box mb={"xs"}>
            <Dependencies
                producerId={building.producerId}
            />
        </Box>

        <Pipeline producerId={building.producerId}/>
    </Page>;
};
